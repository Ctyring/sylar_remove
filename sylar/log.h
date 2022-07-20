#ifndef __SYLAR_LOG_H_
#define __SYLAR_LOG_H_

#include <iostream>
#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>

// 新版的可以用 #once
// namespace是区分其他代码的命名
namespace sylar
{
	class Logger;
	// 日志事件
	// 定义日志的属性
	class LogEvent
	{
	public:
		// c++智能指针
		// 通过计数的方式判断目前使用该资源的指针数，如果为0则销毁指针
		typedef std::shared_ptr<LogEvent> ptr;
		LogEvent(const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time);
		const char *getFile() const { return m_file; }
		int32_t getLine() const { return m_line; }
		uint32_t getElapse() const { return m_elapse; }
		uint32_t getThreadId() const { return m_threadId; }
		uint32_t getFiberId() const { return m_fiberId; }
		uint64_t getTime() const { return m_time; }
		std::string getContent() const { return m_ss.str(); }
		const std::string &getThreadName() const { return m_threadName; }

		std::stringstream &getSS() { return m_ss; }

	private:
		// 文件名 (c++11 新特性：类内初始化)
		const char *m_file = nullptr;
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
		std::stringstream m_ss;
		/// 线程名称
		std::string m_threadName;
	};
	// 日志级别
	class LogLevel
	{
	public:
		enum Level
		{
			UNKNOW = 0,
			DEBUG = 1,
			INFO = 2,
			WARN = 3,
			ERROR = 4,
			FATAL = 5
		};
		static const char *ToString(LogLevel::Level level);
	};

	// 日志输出格式
	class LogFormatter
	{
	public:
		typedef std::shared_ptr<LogFormatter> ptr;
		LogFormatter(const std::string &pattern);

		std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

	public:
		// 子类的基类
		class FormatItem
		{
		public:
			typedef std::shared_ptr<FormatItem> ptr;
			// 将基类的析构函数定义为虚函数，才会调用子类的析构函数
			// 子类析构函数不需要设置为虚函数，就可以调用孙类的析构函数
			virtual ~FormatItem() {}
			// 纯虚函数
			virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
		};

		void init();

	private:
		// formatter 的结构
		std::string m_pattern;
		std::vector<FormatItem::ptr> m_items;
		// 是否有错误
		bool m_error = false;
	};

	// 日志输出地
	class LogAppender
	{
	public:
		typedef std::shared_ptr<LogAppender> ptr;
		// 输出地可以为多个
		virtual ~LogAppender() {}
		virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

		void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
		LogFormatter::ptr getFormatter() const { return m_formatter; }

		void setLevel(LogLevel::Level val) { m_level = val; }
		LogLevel::Level getLevel() const { return m_level; }

	private:
		LogLevel::Level m_level = LogLevel::DEBUG;
		LogFormatter::ptr m_formatter;
	};

	// 日志器
	class Logger : public std::enable_shared_from_this<Logger>
	{
	public:
		typedef std::shared_ptr<Logger> ptr;
		Logger(const std::string &name = "root");
		void log(LogLevel::Level level, const LogEvent::ptr event);
		void debug(LogEvent::ptr event);
		void info(LogEvent::ptr event);
		void warn(LogEvent::ptr event);
		void error(LogEvent::ptr event);
		void fatal(LogEvent::ptr event);

		void addAppender(LogAppender::ptr appender);
		void delAppender(LogAppender::ptr appender);

		LogLevel::Level getLevel() const { return m_level; }
		std::string getName() const { return m_name; }

		void setLevel(LogLevel::Level val) { m_level = val; }

	private:
		// 日志名称
		std::string m_name;
		// 日志级别
		LogLevel::Level m_level;
		// 输出目的地的集合
		std::list<LogAppender::ptr> m_appenders;
		LogFormatter::ptr m_formatter;
	};
	// 输出到控制台的Appender
	class StdoutLogAppender : public LogAppender
	{
	public:
		typedef std::shared_ptr<StdoutLogAppender> ptr;
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
	};
	// 输出到文件的Appender
	class FileLogAppender : public LogAppender
	{
	public:
		typedef std::shared_ptr<FileLogAppender> ptr;
		// 输出到文件需要提供文件名
		FileLogAppender(const std::string &filename);
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
		// 打开文件，成功返回true
		bool reopen();

	private:
		std::string m_filename;
		// 用流的方式写入文件
		std::ofstream m_filestream;
	};
}
#endif
