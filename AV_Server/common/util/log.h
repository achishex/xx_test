#ifndef _LOG_H_MYLOG_I_
#define _LOG_H_MYLOG_I_

#include <string>
#include <string.h>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>

#include "log4cpp/Category.hh"
#include "log4cpp/PropertyConfigurator.hh"	// 配置文件
#include "log4cpp/Priority.hh"				// 优先级

#include "log4cpp/Appender.hh"
//#include "log4cpp/IdsaAppender.hh"		// 发送到IDS或者logger
#include "log4cpp/FileAppender.hh"			// 输出到文件
#include "log4cpp/RollingFileAppender.hh"	// 输出到回卷文件，即当文件到达某个大小后回卷
#include "log4cpp/OstreamAppender.hh"		// 输出到一个ostream类 
#include "log4cpp/RemoteSyslogAppender.hh"	// 输出到远程syslog服务器  
#include "log4cpp/StringQueueAppender.hh"	// 内存队列
#include "log4cpp/SyslogAppender.hh"		// 本地syslog
//#include "log4cpp/Win32DebugAppender.hh"	// 发送到缺省系统调试器		//linux不支持
//#include "log4cpp/NTEventLogAppender.hh"	// 发送到win事件日志		//linux不支持

//Appender type
#include "log4cpp/BasicLayout.hh"		//这个过于简单
#include "log4cpp/PatternLayout.hh"		//建议用这个
#include "log4cpp/SimpleLayout.hh"		//不建议使用

//#include "../../thirdpart/include/log4cpp/BasicLayout.hh"		//这个过于简单
//#include "../../thirdpart/include/log4cpp/PatternLayout.hh"		//建议用这个
//#include "../../thirdpart/include/log4cpp/SimpleLayout.hh"		//不建议使用

#include "configXml.h"

using namespace log4cpp;
using namespace std;

#define FILE_CONENT_BUFFER_MAX	1024 * 64
#define FMT_LEN 2000

class Log
{
public:
    Log();
    Log(const std::string &configFileName);
    Log(const std::string &configFileName, const std::string &instance);
    int init();
    ~Log();

	void emerg(const char* stringFormat, ...);
    void emerg(const std::string& message);

    void fatal(const char* stringFormat, ...);
	void fatal(const std::string& message);

	void alert(const char* stringFormat, ...);
	void alert(const std::string& message);

	void crit(const char* stringFormat, ...);
	void crit(const std::string& message);

	void error(const char* stringFormat, ...);
	void error(const std::string& message);
	
	void warn(const char* stringFormat, ...);
	void warn(const std::string& message);
	
	void notice(const char* stringFormat, ...);
	void notice(const std::string& message);
	
	void info(const char* stringFormat, ...);
	void info(const std::string& message);
		
	void debug(const char* stringFormat, ...);
	void debug(const std::string& message);		

private:
    std::string m_configFileName;
	log4cpp::Category* m_rootCategory;
	log4cpp::Category* m_category;
	std::string m_instance;
};

//#define log_conf_file   "../conf/log.properties"
//static Log AVSLog(log_conf_file);
extern Log *pAvsLog;

#define log_info(fmt, ...)	\
{ \
    char  buffer[FMT_LEN];\
    snprintf(buffer,FMT_LEN,"[%s %s (%d) ]:%s",__FILE__, __FUNCTION__, __LINE__,fmt);\
    if (pAvsLog) { \
        pAvsLog->info(buffer,##__VA_ARGS__);\
    }\
}

#define log_notice(fmt, ...)	\
{ \
    char  buffer[FMT_LEN];\
    snprintf(buffer,FMT_LEN,"[%s %s (%d) ]:%s",__FILE__, __FUNCTION__, __LINE__,fmt);\
    if (pAvsLog) { \
        pAvsLog->notice(buffer,##__VA_ARGS__);\
    }\
}

#define log_error(fmt, ...)	\
{ \
    char  buffer[FMT_LEN];\
    snprintf(buffer,FMT_LEN,"[%s %s (%d) ]:%s",__FILE__, __FUNCTION__, __LINE__,fmt);\
    if (pAvsLog) { \
        pAvsLog->error(buffer,##__VA_ARGS__);\
    }\
}


#define log_debug(fmt, ...)	\
{ \
    char  buffer[FMT_LEN];\
    snprintf(buffer,FMT_LEN,"[%s %s (%d) ]:%s",__FILE__, __FUNCTION__, __LINE__,fmt);\
    if (pAvsLog) { \
        pAvsLog->debug(buffer,##__VA_ARGS__);\
    } \
}

#endif
