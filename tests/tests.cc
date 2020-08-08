#include "vity/log.h"

#include <iostream>

/*
XX(m, MessageFormatItem),
XX(p, LevelFormatItem),
XX(r, ElapseFormatItem),
XX(c, NameFormatItem),
XX(t, ThreadIdFormatItem),
XX(n, NewLineFormatItem),
XX(d, DateTimeFormatItem),
XX(f, FilenameFormatItem),
XX(l, LineFormatItem),
XX(T, TabFormatItem),
XX(F, FiberIdFormatItem),
*/

int main()
{
    vity::Logger::ptr logger(new vity::Logger);
    logger->addAppender(vity::LogAppender::ptr(new vity::StdoutLogAppender));

    vity::FileLogAppender::ptr file_appender(new vity::FileLogAppender("./log.txt"));
    vity::LogFormatter::ptr fmt(new vity::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(vity::LogLevel::ERROR);

    logger->addAppender(file_appender);

    std::cout << "hello vity log" << std::endl;

    VITY_LOG_INFO(logger) << "test macro";
    VITY_LOG_ERROR(logger) << "test macro error";

    
    VITY_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

    auto l = vity::LoggerMgr::GetInstance()->getLogger("xx");
    VITY_LOG_INFO(l) << "xxx";
    return 0;
}