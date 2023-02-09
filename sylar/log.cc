#include "log.h"
#include <string.h>
#include <time.h>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include "config.h"
#include "env.h"
#include "macro.h"
#include "util.h"
#include "util/time.h"

namespace sylar {
const char* LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
#define XX(name)         \
    case LogLevel::name: \
        return #name;    \
        break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX
        default:
            return "UNKNOW";
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, v)            \
    if (str == #v) {            \
        return LogLevel::level; \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOW;
#undef XX
}

LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e) {}

LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream& LogEventWrap::getSS() {
    return m_event->getSS();
}

void LogEvent::format(const char* fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if (len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

void LogAppender::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
    if (m_formatter) {
        m_hasFormatter = true;
    } else {
        m_hasFormatter = false;
    }
}

LogFormatter::ptr LogAppender::getFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

class MessageFormatItem : public LogFormatter::FormatItem {
   public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
   public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
   public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
   public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getLogger()->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
   public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
   public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
   public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
   public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        : m_format(format) {
        if (m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }

   private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
   public:
    FilenameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
   public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
   public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
   public:
    StringFormatItem(const std::string& str) : m_string(str) {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << m_string;
    }

   private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
   public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,
                Logger::ptr logger,
                LogLevel::Level level,
                LogEvent::ptr event) override {
        os << "\t";
    }

   private:
    std::string m_string;
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger,
                   LogLevel::Level level,
                   const char* file,
                   int32_t line,
                   uint32_t elapse,
                   uint32_t thread_id,
                   uint32_t fiber_id,
                   uint64_t time,
                   const std::string& thread_name)
    : m_file(file),
      m_line(line),
      m_elapse(elapse),
      m_threadId(thread_id),
      m_fiberId(fiber_id),
      m_time(time),
      m_threadName(thread_name),
      m_logger(logger),
      m_level(level) {}

Logger::Logger(const std::string& name)
    : m_name(name), m_level(LogLevel::DEBUG) {
    m_formatter.reset(new LogFormatter(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;

    for (auto& i : m_appenders) {
        MutexType::Lock ll(i->m_mutex);
        if (!i->m_hasFormatter) {
            i->m_formatter = m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string& val) {
    std::cout << "---" << val << std::endl;
    sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
    if (new_val->isError()) {
        std::cout << "Logger setFormatter name=" << m_name << " value=" << val
                  << " invalid formatter" << std::endl;
        return;
    }
    // m_formatter = new_val;
    setFormatter(new_val);
}

std::string Logger::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }

    for (auto& i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogFormatter::ptr Logger::getFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

void Logger::addAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    if (!appender->getFormatter()) {
        MutexType::Lock ll(appender->m_mutex);
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if (*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders() {
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level) {
        auto self = shared_from_this();
        MutexType::Lock lock(m_mutex);
        if (!m_appenders.empty()) {
            for (auto& i : m_appenders) {
                i->log(self, level, event);
            }
        } else if (m_root) {
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event) {
    log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event) {
    log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event) {
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event) {
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event) {
    log(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string& filename)
    : m_filename(filename) {
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger,
                          LogLevel::Level level,
                          LogEvent::ptr event) {
    if (level >= m_level) {
        uint64_t now = event->getTime();
        if (now >= (m_lastTime + 3)) {
            reopen();
            m_lastTime = now;
        }
        MutexType::Lock lock(m_mutex);
        // if(!(m_filestream << m_formatter->format(logger, level, event))) {
        if (!m_formatter->format(m_filestream, logger, level, event)) {
            std::cout << "error" << std::endl;
        }
    }
}

std::string FileLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

bool FileLogAppender::reopen() {
    MutexType::Lock lock(m_mutex);
    if (m_filestream) {
        m_filestream.close();
    }
    return FSUtil::OpenForWrite(m_filestream, m_filename, std::ios::app);
}
TimeSlicingFileLogAppender::TimeSlicingFileLogAppender(
    const std::string& path,
    const std::string& prefix,
    const std::string& suffix,
    uint64_t interval,
    std::string timeformat,
    std::string beginTime)
    : m_path(path),
      m_prefix(prefix),
      m_suffix(suffix),
      m_interval(interval),
      m_fmt(timeformat) {
    if (beginTime.empty()) {
        m_lastTime = time(0);
    } else {
        m_lastTime = ParseTime(beginTime, m_fmt);
    }
    reopen(uint64_t(time(0)));
}
void TimeSlicingFileLogAppender::log(Logger::ptr logger,
                                     LogLevel::Level level,
                                     LogEvent::ptr event) {
    if (level >= m_level) {
        uint64_t now = event->getTime();
        if (now >= (m_lastTime + m_interval)) {
            reopen(now);
        }
        MutexType::Lock lock(m_mutex);
        if (!m_formatter->format(m_filestream, logger, level, event)) {
            std::cout << "error" << std::endl;
        }
    }
}

std::string TimeSlicingFileLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["path"] = m_path;
    node["prefix"] = m_prefix;
    node["suffix"] = m_suffix;
    node["interval"] = m_interval;
    // beginTime 需要转化为年月日时分秒
    std::string beginTime = FormatTime(m_lastTime, m_fmt);
    node["beginTime"] = m_lastTime;
    if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

bool TimeSlicingFileLogAppender::reopen(uint64_t now) {
    MutexType::Lock lock(m_mutex);
    if (m_filestream) {
        m_filestream.close();
    }
    // 将时间戳now格式化为年月日时分秒
    struct tm tm;
    time_t time = now;
    localtime_r(&time, &tm);
    std::stringstream ss;
    std::string filename =
        m_path + "/" + m_prefix + "-" + FormatTime(now, m_fmt) + m_suffix;
    return FSUtil::OpenForWrite(m_filestream, filename, std::ios::app);
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger,
                            LogLevel::Level level,
                            LogEvent::ptr event) {
    if (level >= m_level) {
        MutexType::Lock lock(m_mutex);
        m_formatter->format(std::cout, logger, level, event);
    }
}

std::string StdoutLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogFormatter::LogFormatter(const std::string& pattern) : m_pattern(pattern) {
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                 LogLevel::Level level,
                                 LogEvent::ptr event) {
    std::stringstream ss;
    for (auto& i : m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& ofs,
                                   std::shared_ptr<Logger> logger,
                                   LogLevel::Level level,
                                   LogEvent::ptr event) {
    for (auto& i : m_items) {
        i->format(ofs, logger, level, event);
    }
    return ofs;
}

//%xxx %xxx{xxx} %%
void LogFormatter::init() {
    // str, format, type
    // 这个三元组参数：str是原始字符串，format是字符串的格式化方法（比如时间，有很多显示方法，这个内容就在{之间}），type代表是否格式化(0代表普通字符串)
    std::vector<std::tuple<std::string, std::string, int> > vec;
    std::string nstr;
    for (size_t i = 0; i < m_pattern.size(); ++i) {
        // 如果不是%
        if (m_pattern[i] != '%') {
            // 先放入nstr中
            nstr.append(1, m_pattern[i]);
            continue;
        }
        // 如果是%并且不是最后一个字符
        if ((i + 1) < m_pattern.size()) {
            // 如果下一个字符是%，说明是%%，放入一个%到nstr
            if (m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }
        // 到这里说明目前的字符是%并且下一个字符不是%
        size_t n = i + 1;
        // 1说明'{'没有'}'匹配
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while (n < m_pattern.size()) {
            // 如果是无效字符，说明是普通字符，就切出去
            if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' &&
                                m_pattern[n] != '}')) {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if (fmt_status == 0) {
                if (m_pattern[n] == '{') {
                    // 把大括号之前的部分切割
                    str = m_pattern.substr(i + 1, n - i - 1);
                    // std::cout << "*" << str << std::endl;
                    fmt_status = 1;  // 解析格式
                                     //  大括号开始
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if (fmt_status == 1) {
                if (m_pattern[n] == '}') {
                    // 切割大括号内的字符串
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    // std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if (n == m_pattern.size()) {
                // 如果到结尾都没切割到字符串，就把%后面的所有字符都切割出来
                if (str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if (fmt_status == 0) {
            if (!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if (fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - "
                      << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }
    // 如果还有普通字符串进来的话，放入数组
    if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    // 到这里为止，已经把原模式串切割成了很多三元组，每个三元组代表一个格式化的内容
    // 这里相当于一个字典，key是str，value是对应的格式化方法
    static std::map<std::string,
                    std::function<FormatItem::ptr(const std::string& str)> >
        s_format_items = {
#define XX(str, C)                                                             \
    {                                                                          \
#str,                                                                  \
            [](const std::string& fmt) { return FormatItem::ptr(new C(fmt)); } \
    }

            XX(m, MessageFormatItem),     // m:消息
            XX(p, LevelFormatItem),       // p:日志级别
            XX(r, ElapseFormatItem),      // r:累计毫秒数
            XX(c, NameFormatItem),        // c:日志名称
            XX(t, ThreadIdFormatItem),    // t:线程id
            XX(n, NewLineFormatItem),     // n:换行
            XX(d, DateTimeFormatItem),    // d:时间
            XX(f, FilenameFormatItem),    // f:文件名
            XX(l, LineFormatItem),        // l:行号
            XX(T, TabFormatItem),         // T:Tab
            XX(F, FiberIdFormatItem),     // F:协程id
            XX(N, ThreadNameFormatItem),  // N:线程名称
#undef XX
        };

    // 遍历三元组
    for (auto& i : vec) {
        // 如果是普通字符串
        if (std::get<2>(i) == 0) {
            m_items.push_back(
                FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            // 查找对应的格式化方法
            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(
                    "<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                // 把格式化方法传给方法
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }

        // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i)
        // << ")
        // - (" << std::get<2>(i) << ")" << std::endl;
    }
    // std::cout << m_items.size() << std::endl;
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    m_loggers[m_root->m_name] = m_root;

    init();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    if (it != m_loggers.end()) {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

struct LogAppenderDefine {
    int type = 0;  // 1 File, 2 Stdout 3 时间分片
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;
    /// 文件路径
    std::string path;
    /// 日志所在文件夹
    std::string prefix;
    /// 文件路径后缀
    std::string suffix;
    /// 开始时间
    std::string beginTime;
    /// 间隔时间
    uint64_t interval;
    /// 时间格式
    std::string fmt;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type && level == oth.level &&
               formatter == oth.formatter && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name && level == oth.level &&
               formatter == oth.formatter && appenders == appenders;
    }

    bool operator<(const LogDefine& oth) const { return name < oth.name; }

    bool isValid() const { return !name.empty(); }
};

template <>
class LexicalCast<std::string, LogDefine> {
   public:
    LogDefine operator()(const std::string& v) {
        YAML::Node n = YAML::Load(v);
        LogDefine ld;
        if (!n["name"].IsDefined()) {
            std::cout << "log config error: name is null, " << n << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = n["name"].as<std::string>();
        ld.level = LogLevel::FromString(
            n["level"].IsDefined() ? n["level"].as<std::string>() : "");
        if (n["formatter"].IsDefined()) {
            ld.formatter = n["formatter"].as<std::string>();
        }
        if (n["appenders"].IsDefined()) {
            // std::cout << "==" << ld.name << " = " << n["appenders"].size() <<
            // std::endl;
            for (size_t x = 0; x < n["appenders"].size(); ++x) {
                auto a = n["appenders"][x];
                if (!a["type"].IsDefined()) {
                    std::cout << "log config error: appender type is null, "
                              << a << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if (type == "FileLogAppender") {
                    lad.type = 1;
                    if (!a["file"].IsDefined()) {
                        std::cout
                            << "log config error: fileappender file is null, "
                            << a << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if (a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else if (type == "StdoutLogAppender") {
                    lad.type = 2;
                } else if (type == "TimeSlicingFileLogAppender") {
                    lad.type = 3;
                    if (!a["path"].IsDefined()) {
                        std::cout << "log config error: "
                                     "fileappender path "
                                     "is null, "
                                  << a << std::endl;
                        continue;
                    } else {
                        lad.path = a["path"].as<std::string>();
                    }
                    if (!a["prefix"].IsDefined()) {
                        std::cout << "log config error: "
                                     "fileappender prefix "
                                     "is null, "
                                  << a << std::endl;
                        continue;
                    } else {
                        lad.prefix = a["prefix"].as<std::string>();
                    }
                    if (a["suffix"].IsDefined()) {
                        lad.suffix = a["suffix"].as<std::string>();
                    } else {
                        lad.suffix = ".log";
                    }
                    if (a["beginTime"].IsDefined()) {
                        lad.beginTime = a["beginTime"].as<std::string>();
                    } else {
                        lad.beginTime = time(0);
                    }
                    if (a["interval"].IsDefined()) {
                        lad.interval = a["interval"].as<uint64_t>();
                    } else {
                        lad.interval = 60 * 60 * 24;
                    }
                    if (a["fmt"].IsDefined()) {
                        lad.fmt = a["fmt"].as<std::string>();
                    } else {
                        lad.fmt = "%Y-%m-%d %H:%M:%S";
                    }
                } else {
                    std::cout << "log config error: appender type is invalid, "
                              << a << std::endl;
                    continue;
                }

                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template <>
class LexicalCast<LogDefine, std::string> {
   public:
    std::string operator()(const LogDefine& i) {
        YAML::Node n;
        n["name"] = i.name;
        if (i.level != LogLevel::UNKNOW) {
            n["level"] = LogLevel::ToString(i.level);
        }
        if (!i.formatter.empty()) {
            n["formatter"] = i.formatter;
        }
        for (auto& a : i.appenders) {
            YAML::Node na;
            if (a.type == 1) {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            } else if (a.type == 2) {
                na["type"] = "StdoutLogAppender";
            } else if (a.type == 3) {
                na["type"] = "TimeSlicingFileLogAppender";
                na["path"] = a.path;
                na["prefix"] = a.prefix;
                na["suffix"] = a.suffix;
                na["beginTime"] = a.beginTime;
                na["interval"] = a.interval;
                na["fmt"] = a.fmt;
            }
            if (a.level != LogLevel::UNKNOW) {
                na["level"] = LogLevel::ToString(a.level);
            }

            if (!a.formatter.empty()) {
                na["formatter"] = a.formatter;
            }

            n["appenders"].push_back(na);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

sylar::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =
    sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine>& old_value,
                                      const std::set<LogDefine>& new_value) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";
            for (auto& i : new_value) {
                auto it = old_value.find(i);
                sylar::Logger::ptr logger;
                if (it == old_value.end()) {
                    // 新增logger
                    logger = SYLAR_LOG_NAME(i.name);
                } else {
                    if (!(i == *it)) {
                        // 修改的logger
                        logger = SYLAR_LOG_NAME(i.name);
                    }
                }
                logger->setLevel(i.level);
                if (!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();
                for (auto& a : i.appenders) {
                    sylar::LogAppender::ptr ap;
                    if (a.type == 1) {
                        ap.reset(new FileLogAppender(a.file));
                    } else if (a.type == 2) {
                        if (!sylar::EnvMgr::GetInstance()->has("d")) {
                            ap.reset(new StdoutLogAppender);
                        } else {
                            continue;
                        }
                    } else if (a.type == 3) {
                        ap.reset(new TimeSlicingFileLogAppender(
                            a.path, a.prefix, a.suffix, a.interval, a.fmt,
                            a.beginTime));
                    }
                    ap->setLevel(a.level);
                    if (!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if (!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "log.name=" << i.name
                                      << " appender type=" << a.type
                                      << " formatter=" << a.formatter
                                      << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }

            for (auto& i : old_value) {
                auto it = new_value.find(i);
                if (it == new_value.end()) {
                    // 删除logger
                    auto logger = SYLAR_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)0);
                    logger->clearAppenders();
                }
            }
        });
    }
};

static LogIniter __log_init;

std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for (auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void LoggerManager::init() {}

}  // namespace sylar