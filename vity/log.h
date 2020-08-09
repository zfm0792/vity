#ifndef __VITY_LOG_H__
#define __VITY_LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream>
#include <stdarg.h>
#include <map>
#include "utils.h"
#include "singleton.h"

#define VITY_LOG_LEVEL(logger,level) \
    if(logger->getLevel() <= level) \
        vity::LogEventWrap(vity::LogEvent::ptr(new vity::LogEvent(logger,level, \
                            __FILE__,__LINE__,0,vity::GetThreadId(),\
                            vity::GetFiberId(),time(0),vity::GetThreadName()))).getSS()

#define VITY_LOG_DEBUG(logger) VITY_LOG_LEVEL(logger,vity::LogLevel::DEBUG)
#define VITY_LOG_INFO(logger) VITY_LOG_LEVEL(logger,vity::LogLevel::INFO)
#define VITY_LOG_WARN(logger) VITY_LOG_LEVEL(logger,vi ty::LogLevel::WARN)
#define VITY_LOG_ERROR(logger) VITY_LOG_LEVEL(logger,vity::LogLevel::ERROR)
#define VITY_LOG_FATAL(logger) VITY_LOG_LEVEL(logger,vity::LogLevel::FATAL)


#define VITY_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        vity::LogEventWrap(vity::LogEvent::ptr(new vity::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, vity::GetThreadId(),\
                vity::GetFiberId(), time(0),vity::GetThreadName()))).getEvent()->format(fmt, __VA_ARGS__)

#define VITY_LOG_FMT_DEBUG(logger, fmt, ...) VITY_LOG_FMT_LEVEL(logger, vity::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define VITY_LOG_FMT_INFO(logger, fmt, ...)  VITY_LOG_FMT_LEVEL(logger, vity::LogLevel::INFO, fmt, __VA_ARGS__)
#define VITY_LOG_FMT_WARN(logger, fmt, ...)  VITY_LOG_FMT_LEVEL(logger, vity::LogLevel::WARN, fmt, __VA_ARGS__)
#define VITY_LOG_FMT_ERROR(logger, fmt, ...) VITY_LOG_FMT_LEVEL(logger, vity::LogLevel::ERROR, fmt, __VA_ARGS__)
#define VITY_LOG_FMT_FATAL(logger, fmt, ...) VITY_LOG_FMT_LEVEL(logger, vity::LogLevel::FATAL, fmt, __VA_ARGS__)

// 获取主日志器
#define VITY_LOG_ROOT() vity::LoggerMgr::GetInstance()->getRoot()
// 获取name的日志器
#define VITY_LOG_NAME(name) vity::LoggerMgr::GetInstance()->getLogger(name)

namespace vity{

class Logger;
class LoggerManager;

// 日志级别
class LogLevel{
public:
    // 日志级别枚举
    enum Level{
        UNKNOW = 0, // 未知
        DEBUG = 1,  // DEBUG
        INFO = 2,   // INFO
        WARN = 3,   // WARN
        ERROR = 4,  // ERROR
        FATAL = 5   // FATAL
    };

    // 将日志级别转换成文本输出
    // param[in] level 日志级别
    static const char* ToString(LogLevel::Level level);

    // 将文本转换为日志级别
    // param[in] str 日志级别文本
    static LogLevel::Level FromString(const std::string& str);
};
// 日志事件
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;

    // 构造函数
    // param[in] logger 日志器 level 日志级别 file 文件名 line 文件行号 elapse 程序启动依赖耗时
    // thread_id 线程id firber_id 协程id time 日志事件 thread_name 线程名称
    LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level,
            const char* file,int32_t line,uint32_t elapse,uint32_t thread_id,
            uint32_t fiber_id,uint64_t time,const std::string& thread_name);

    // 返回文件名
    const char* getFile() const { return m_file; }

    // 返回文件行号
    int32_t getLine() const { return m_line; }

    // 返回耗时
    uint32_t getElapse() const { return m_elapse; }

    // 返回线程id
    uint32_t getThreadId() const { return m_threadId; }

    // 返回线程名称
    const std::string& getThreadName() const { return m_threadName; }

    // 返回协程id
    uint32_t getFiberId() const { return m_fiberId; }

    // 返回时间
    uint64_t getTime() const { return m_time; }

    // 返回日志内容
    std::string getContent() const { return m_ss.str(); }

    // 返回日志器
    std::shared_ptr<Logger> getLogger() const { return m_logger; }

    // 返回日级别
    LogLevel::Level getLevel() const { return m_level; }

    // 返回日志内容字符串流
    std::stringstream& getSS() { return m_ss; }

    // 格式化写入日志内容
    void format(const char* fmt,...);

    // 格式化写入日志内容
    void format(const char* fmt,va_list al);

private:
    const char* m_file = nullptr;   // 文件名
    int32_t m_line = 0;             // 行号
    uint32_t m_elapse = 0;          // 程序启动到现在经过的毫秒数
    uint32_t m_threadId = 0;        // 线程id
    std::string m_threadName;
    uint32_t m_fiberId = 0;         // 协程id
    uint64_t m_time = 0;            // 时间戳
    std::stringstream m_ss; 

    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;

};

// vity::LogEventWrap(event)
// ~LogEventWrap() {event->getLogger()->log(level,event)}
// 日志时间包装器
class LogEventWrap {
public:
    // 构造函数
    // param[in] e 日志事件
    LogEventWrap(LogEvent::ptr e);

    // 析构函数
    ~LogEventWrap();

    // 获取日志事件
    LogEvent::ptr getEvent() const { return m_event;}

    // 获取日志内容
    std::stringstream& getSS();
private:

    // 日志事件
    LogEvent::ptr m_event;
};


// 日志格式 -- 抽象类  日志格式器 多种格式类型 
// 线程 时间  文件行 ... 不确定的格式
// 对格式进行解析

// 日志格式化
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    // 构造函数
    // param[in] pattern 格式模板
    // details
    // %m 消息
    // %p 日志级别
    // %r 累计毫秒数
    // %c 日志名称
    // %t 线程id
    // %n 换行
    // %d 事件
    // %f 文件名
    // %l 行号
    // %T 制表符
    // %F 协程id
    // %N 线程名称
    // 默认格式: %d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
    LogFormatter(const std::string& pattern);

    // 返回格式化日志文本
    // param[in] logger 日志器  level 日志级别 event 日志事件
    std::string format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event);
public:
    // 日志内容项格式化
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        // 析构函数
        virtual ~FormatItem(){}
        // 格式化到日志流
        // param[in,out] os 日志输出流
        // param[in] logger 日志器  level 日志级别 event 日志事件
        virtual void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) = 0;
    };
    // 初始化 解析日志模板
    void init();

    // 是否有错误
    bool isError() const { return m_error; }

    // 返回日志模板
    const std::string getPattern() const { return m_pattern; }
private:
    // pattern 日志格式模板
    std::string m_pattern;
    // 日志解析后格式
    std::vector<FormatItem::ptr> m_items;
    // 是否有错误
    bool m_error = false;
};

// 日志输出地 -- 抽象类 需要 一个默认格式  输出的日志等级
// 日志输出目标
class LogAppender{
friend class Logger;
public:
    typedef std::shared_ptr<LogAppender> ptr;

    // 析构函数
    virtual ~LogAppender() {}

    // 写入日志
    // param[in] logger 日志器  level 日志级别 event 日志事件
    virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) = 0;

    // 将日志输出目标配置转成YAML String
    virtual std::string toYamlString() = 0;

    // 更改日志格式器
    void setFormatter(LogFormatter::ptr val){
        m_formatter = val;
        if(m_formatter){
            m_hasFormatter = true;
        }else{
            m_hasFormatter = false;
        }
    }
    // 获取日志格式器
    LogFormatter::ptr getFormatter() const { return m_formatter; }

    // 获取日志等级
    LogLevel::Level getLevel() const { return m_level; }
    // 设置日志等级
    void setLevel(LogLevel::Level val) { m_level = val; }
protected:
    LogLevel::Level m_level = LogLevel::DEBUG; // 日志等级
    LogFormatter::ptr m_formatter; // 日志格式
    // 是否有自己的日志格式器
    bool m_hasFormatter = false;
};

// 日志器 -- 输出  即输出到每个logappender中  最终使用的其实就是日志器
// 观察者模式 --- 观察者  被观察的事件 一个抽象的观察事件
// 观察者里面必须要又一个抽象的被观察者对象的存储结构
// 日志要输出  那么可以是 文件  标准输出  
// 定义了一个LogAppender 抽象类 ---- 待观察的事件
// Logger 观察者 
// 日志器
class Logger : public std::enable_shared_from_this<Logger>
{
    friend class LogManager;
public:
    typedef std::shared_ptr<Logger> ptr;

    // 构造函数
    // param[in] name 日志器名称
    Logger(const std::string& name = "root");
    // 写日志 level 日志级别  event 日志事件
    void log(LogLevel::Level level,LogEvent::ptr event);
    // 写debug 级别日志
    void debug(LogEvent::ptr event);
    // 写info 级别日志
    void info(LogEvent::ptr event);
    // 写warn 级别日志
    void warn(LogEvent::ptr event);
    // 写error 级别日志
    void error(LogEvent::ptr event);
    // 写fatal 级别日志
    void fatal(LogEvent::ptr event);

    // 添加日志输出目标
    void addAppender(LogAppender::ptr appender);
    // 删除日志目标
    void delAppender(LogAppender::ptr appender);
    // 清空日志目标
    void clearAppenders();
    // 返回日志级别
    LogLevel::Level getLevel() const { return m_level; }
    // 设置日志级别
    void setLevel(LogLevel::Level val) { m_level = val; }
    // 返回日志名称
    const std::string& getName() const { return m_name; }
    // 设置日志格式器
    void setFormatter(LogFormatter::ptr val);
    // 设置日志格式模板
    void setFormatter(const std::string& val);
    // 获取日志格式器
    LogFormatter::ptr getFormatter();

    // 将日志器的配置转成YAML
    std::string toYamlString();
private:
    std::string m_name; // 日志名称
    LogLevel::Level m_level; //日志级别  只有达到了该等级 才会被输出
    std::list<LogAppender::ptr> m_appenders; //Appender集合
    LogFormatter::ptr m_formatter; // 日志格式
    Logger::ptr m_root; // 主日志器
};

// 输出到控制台的Appender
class StdoutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override;
    std::string toYamlString() override;
};

// 定义输出到文件的Appender
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(Logger::ptr logger,LogLevel::Level level,LogEvent::ptr event) override;
    std::string toYamlString() override;

    // 重新打开日志文件
    bool reopen();
private:
    // 文件路径
    std::string m_filename;
    // 文件流
    std::ofstream m_filestream;
};
// 日志器管理类
class LogManager{
public:
    // 构造函数
    LogManager();
    // 获取日志器
    // param[in] name 日志器名称
    Logger::ptr getLogger(const std::string& name);
    // 初始化
    void init();
    // 返回主日志器
    Logger::ptr getRoot() const { return m_root; }
    // 将所有日志器配置转成YAML String
    std::string toYamlString();

private:
    // 日志器容器
    std::map<std::string,Logger::ptr> m_loggers;
    // 主日志器
    Logger::ptr m_root;
};
// 日志器管理类单例模式
typedef Singleton<LogManager> LoggerMgr;
}

#endif