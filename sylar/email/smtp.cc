#include "smtp.h"
#include "sylar/log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

SmtpClient::SmtpClient(Socket::ptr sock) : sylar::SocketStream(sock) {}

SmtpClient::ptr SmtpClient::Create(const std::string& host,
                                   uint32_t port,
                                   bool ssl) {
    sylar::IPAddress::ptr addr = sylar::Address::LookupAnyIPAddress(host);
    if (!addr) {
        SYLAR_LOG_ERROR(g_logger)
            << "invalid smtp server: " << host << ":" << port << " ssl=" << ssl;
        return nullptr;
    }
    addr->setPort(port);
    Socket::ptr sock;
    if (ssl) {
        sock = sylar::SSLSocket::CreateTCP(addr);
    } else {
        sock = sylar::Socket::CreateTCP(addr);
    }
    if (!sock->connect(addr)) {
        SYLAR_LOG_ERROR(g_logger) << "connect smtp server: " << host << ":"
                                  << port << " ssl=" << ssl << " fail";
        return nullptr;
    }
    std::string buf;
    buf.resize(1024);

    SmtpClient::ptr rt(new SmtpClient(sock));
    int len = rt->read(&buf[0], buf.size());
    if (len <= 0) {
        return nullptr;
    }
    buf.resize(len);
    if (sylar::TypeUtil::Atoi(buf) != 220) {
        return nullptr;
    }
    rt->m_host = host;
    return rt;
}

SmtpResult::ptr SmtpClient::doCmd(const std::string& cmd, bool debug) {
    if (writeFixSize(cmd.c_str(), cmd.size()) <= 0) {
        return std::make_shared<SmtpResult>(SmtpResult::IO_ERROR,
                                            "write io error");
    }
    std::string buf;
    buf.resize(4096);
    auto len = read(&buf[0], buf.size());
    if (len <= 0) {
        return std::make_shared<SmtpResult>(SmtpResult::IO_ERROR,
                                            "read io error");
    }
    buf.resize(len);
    if (debug) {
        m_ss << "C: " << cmd;
        m_ss << "S: " << buf;
    }

    int code = sylar::TypeUtil::Atoi(buf);
    if (code >= 400) {
        return std::make_shared<SmtpResult>(
            code, sylar::replace(buf.substr(buf.find(' ') + 1), "\r\n", ""));
    }
    return nullptr;
}

SmtpResult::ptr SmtpClient::send(EMail::ptr email,
                                 int64_t timeout_ms,
                                 bool debug) {
#define DO_CMD()                \
    result = doCmd(cmd, debug); \
    if (result) {               \
        return result;          \
    }

    Socket::ptr sock = getSocket();
    if (sock) {
        sock->setRecvTimeout(timeout_ms);
        sock->setSendTimeout(timeout_ms);
    }

    SmtpResult::ptr result;
    // 发送HELO命令标识发件人自己的身份 HELO smtp.163.com 服务端会返回 250 OK
    std::string cmd = "HELO " + m_host + "\r\n";
    DO_CMD();
    if (!m_authed && !email->getFromEMailAddress().empty()) {
        // 发送AUTH LOGIN命令，表示开始进行登录验证 服务端会返回 334
        // dXNlcm5hbWU6
        cmd = "AUTH LOGIN\r\n";
        DO_CMD();
        // 发送用户名信息的base64编码 服务端会返回 334 UGFzc3dvcmQ6
        auto pos = email->getFromEMailAddress().find('@');
        cmd = sylar::base64encode(email->getFromEMailAddress().substr(0, pos)) +
              "\r\n";

        DO_CMD();
        // 发送密码信息的base64编码 服务端会返回 235 Authentication successful
        cmd = sylar::base64encode(email->getFromEMailPasswd()) + "\r\n";
        DO_CMD();

        m_authed = true;
    }
    // 发送MAIL FROM命令，表示发件人的邮箱地址 MAIL FROM: <发件人邮箱地址>
    // 服务端会返回 250 Mail OK
    cmd = "MAIL FROM: <" + email->getFromEMailAddress() + ">\r\n";
    DO_CMD();
    std::set<std::string> targets;
#define XX(fun)                    \
    for (auto& i : email->fun()) { \
        targets.insert(i);         \
    }
    XX(getToEMailAddress);
    XX(getCcEMailAddress);
    XX(getBccEMailAddress);
#undef XX
    // 发送RCPT TO命令，表示收件人的邮箱地址 RCPT TO: <收件人邮箱地址>
    // 服务端会返回 250 Mail OK 可以有多个收件人
    for (auto& i : targets) {
        cmd = "RCPT TO: <" + i + ">\r\n";
        DO_CMD();
    }

    // 发送DATA命令，表示开始发送邮件内容 DATA 服务端会返回 354 End data with
    // <CR><LF>.<CR><LF>
    // 表示当我们希望结束邮件内容的输入时，需要在一行单独输入一个 .
    // 并且以回车换行结束 即 \r\n. \r\n
    cmd = "DATA\r\n";
    DO_CMD();

    auto& entitys = email->getEntitys();
    // 接下来拼接邮件内容
    std::stringstream ss;
    // 邮件头 格式为 From: <发件人邮箱地址> To: <收件人邮箱地址> Subject:
    // 邮件标题
    ss << "From: <" << email->getFromEMailAddress() << ">\r\n"
       << "To: ";
#define XX(fun)                                 \
    do {                                        \
        auto& v = email->fun();                 \
        for (size_t i = 0; i < v.size(); ++i) { \
            if (i) {                            \
                ss << ",";                      \
            }                                   \
            ss << "<" << v[i] << ">";           \
        }                                       \
        if (!v.empty()) {                       \
            ss << "\r\n";                       \
        }                                       \
    } while (0);
    XX(getToEMailAddress);
    if (!email->getCcEMailAddress().empty()) {
        ss << "Cc: ";
        XX(getCcEMailAddress);
    }
    ss << "Subject: " << email->getTitle() << "\r\n";
    std::string boundary;
    // 如果有附件就需要生成一个boundary 用于分隔邮件内容的每个部分
    if (!entitys.empty()) {
        boundary = sylar::random_string(16);
        ss << "Content-Type: multipart/mixed;boundary=" << boundary << "\r\n";
    }
    ss << "MIME-Version: 1.0\r\n";
    // 分隔符
    if (!boundary.empty()) {
        ss << "\r\n--" << boundary << "\r\n";
    }
    // 文本信息
    ss << "Content-Type: text/html;charset=\"utf-8\"\r\n"
       << "\r\n"
       << email->getBody() << "\r\n";
    // 附件信息
    for (auto& i : entitys) {
        ss << "\r\n--" << boundary << "\r\n";
        ss << i->toString();
    }
    if (!boundary.empty()) {
        ss << "\r\n--" << boundary << "--\r\n";
    }
    ss << "\r\n.\r\n";
    cmd = ss.str();
    DO_CMD();
#undef XX
#undef DO_CMD
    return std::make_shared<SmtpResult>(0, "ok");
}

std::string SmtpClient::getDebugInfo() {
    return m_ss.str();
}

}  // namespace sylar