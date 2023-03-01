#include "http_session.h"
#include "http_parser.h"
namespace sylar {
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("debug");
namespace http {
HttpSession::HttpSession(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner) {}
HttpRequest::ptr HttpSession::recvRequest() {
    // 创建解析器
    HttpRequestParser::ptr parser(new HttpRequestParser);
    // 获取解析的缓存大小(这个是自定义的全局值)
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    // uint64_t buff_size = 100;
    // 创建char数组
    std::shared_ptr<char> buffer(new char[buff_size],
                                 [](char* ptr) { delete[] ptr; });
    char* data = buffer.get();
    // SYLAR_LOG_INFO(g_logger) << "buff_size=" << buff_size << std::endl;
    // 偏移量
    int offset = 0;
    do {
        // 读取数据
        int len = read(data + offset, buff_size - offset);
        SYLAR_LOG_INFO(g_logger) << "len=" << len << std::endl;
        // 如果读完了就关闭
        if (len <= 0) {
            // SYLAR_LOG_INFO(g_logger) << "len <= 0" << std::endl;
            close();
            return nullptr;
        }
        // 得到数据总长度
        len += offset;
        // 解析
        size_t nparse = parser->execute(data, len);
        if (parser->hasError()) {
            close();
            return nullptr;
        }
        // 重新计算偏移量，为下一次读取做准备
        offset = len - nparse;
        // 如果偏移量等于缓存大小，说明读完了，关闭(这里设定就是，加入特别大，默认为读完了)
        if (offset == (int)buff_size) {
            // SYLAR_LOG_INFO(g_logger) << "offset == buff_size" << std::endl;
            close();
            return nullptr;
        }
        if (parser->isFinished()) {
            break;
        }
    } while (true);
    // 单独处理消息体
    int64_t length = parser->getContentLength();
    if (length > 0) {
        std::string body;
        body.resize(length);
        int len = 0;
        // 如果偏移量比消息体长度大，说明消息体已经被读出来了
        if (length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;
        // 如果消息体还有部分没被读出来
        if (length > 0) {
            // 继续读取
            if (readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        // 设置消息体
        parser->getData()->setBody(body);
    }
    // 处理长连接
    std::string keep_alive = parser->getData()->getHeader("Connection");
    if (!strcasecmp(keep_alive.c_str(), "keep-alive")) {
        parser->getData()->setClose(false);
    }
    return parser->getData();
}
int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}
}  // namespace http
}  // namespace sylar