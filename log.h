#ifndef __SYLAR_LOG_H_
#define __SYLAR_LOG_H_

#include <string>
#include <stdint.h>
#include <memory>
// 新版的可以用 #once
// namespace是区分其他代码的命名
namespace sylar {
// 日志事件
// 定义日志的属性
class LogEvent{
public:
	// c++智能指针
	// 通过计数的方式判断目前使用该资源的指针数，如果为0则销毁指针
	typedef std::shared_ptr<LogEvent> ptr;
	LogEvent();
private:
	// 文件名 (c++11 新特性：类内初始化)
	const char* m_file = nullptr;
	// 行号
	int32_t m_line = 0;
	// 程序启动到现在的毫秒数
	uint32_t m_elapse = 0;
	// 线程号
	uint32_t m_threadId = 0;
	// 协程号
	uint32_t m_fiberId = 0;
	// 时间
	uint64_t m_time;
	// 消息内容
	std::string m_content;
};
// 日志级别
class LogLevel {
public:
	enum Level{
		DEBUG = 1;
		INFO = 2;
		WARN = 3;
		ERROR = 4;
		FATAL = 5;
	};
};

class LogFormmater{
public:
	typedef std::shared_ptr<LogFormater> ptr;
	std::string format(LogEvent::ptr event);
private:
};

// 日志输出地
class LogAppender{
public:
	typedef std::shared_ptr<LogAppender> ptr;
	// 输出地可以为多个
	virtual ~LogAppender() {}	
	void log(LogLevel::Level level, LogEvent::ptr event);
private:
	LogLevel::level m_level;

};

// 日志器
class Logger{
public:
	typedef std::shared_ptr<Logger> ptr;
	Logger(const std::string& name = "root");
	void log(LogLevel::Level level, const LogEvent::ptr event);
private:
	std::string m_name;
	logLevel::Level m_level;
	LogAppender::ptr;
};

class StdoutLogAppender : public LogAppender{
};

class FileLogAppender : LogAppender{
};

#endif
