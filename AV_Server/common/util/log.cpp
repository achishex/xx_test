#include "log.h"

const string file_name = "../conf/log.properties";

Log::Log()
  : m_configFileName(file_name)
  , m_instance("sample")
{
	init();
}

Log::Log(const std::string &configFileName)
  : m_configFileName(configFileName)
  , m_instance("sample")
{
	//m_instance = "sample";
	init();
}

Log::Log(const std::string &configFileName, const std::string &instance)
  : m_configFileName(configFileName)
  , m_instance(instance)
{
	init();
}

int Log::init()
{
	// 读取解析配置文件
    try
    {
        log4cpp::PropertyConfigurator::configure(m_configFileName);
    }
    catch (log4cpp::ConfigureFailure& ff)
    {
        std::cerr << "log4cpp::PropertyConfigurator::configure(): " << ff.what() << std::endl;
        return -1;
    }

	m_rootCategory = &log4cpp::Category::getRoot();
	m_category = NULL;
	m_category = &log4cpp::Category::getInstance(m_instance);
//	Category::getRoot();
//	Category::getInstance(m_instance);

	return 0;
}

Log::~Log()
{
	log4cpp::Category::shutdown();
}

void Log::emerg(const char* stringFormat, ...)
{
	if(m_category)
	{
		va_list argp;
		char ch[FILE_CONENT_BUFFER_MAX];
		va_start(argp, stringFormat);
		vsprintf(ch, stringFormat, argp);
		va_end(argp);
		m_category->emerg(ch);
	}
}

void Log::emerg(const std::string& message)
{
	if(m_category)
	{
		m_category->emerg(message);
	}
}

void Log::fatal(const char* stringFormat, ...)
{
	if(m_category)
	{
		va_list argp;
		char ch[FILE_CONENT_BUFFER_MAX];
		va_start(argp, stringFormat);
		vsprintf(ch, stringFormat, argp);
		va_end(argp);
		m_category->fatal(ch);
	}
}

void Log::fatal(const std::string& message)
{
	if(m_category)
	{
		m_category->fatal(message);
	}
}

void Log::alert(const char* stringFormat, ...)
{
	if(m_category)
	{
		va_list argp;
		char ch[FILE_CONENT_BUFFER_MAX];
		va_start(argp, stringFormat);
		vsprintf(ch, stringFormat, argp);
		va_end(argp);
		m_category->alert(ch);
	}
}

void Log::alert(const std::string& message)
{
	if(m_category)
	{
		m_category->alert(message);
	}
}

void Log::crit(const char* stringFormat, ...)
{
	if(m_category)
	{
		va_list argp;
		char ch[FILE_CONENT_BUFFER_MAX];
		va_start(argp, stringFormat);
		vsprintf(ch, stringFormat, argp);
		va_end(argp);
		m_category->crit(ch);
	}
}

void Log::crit(const std::string& message)
{
	if(m_category)
	{
		m_category->crit(message);
	}
}

void Log::error(const char* stringFormat, ...)
{
	if(m_category)
	{
		va_list argp;
		char ch[FILE_CONENT_BUFFER_MAX];
		va_start(argp, stringFormat);
		vsprintf(ch, stringFormat, argp);
		va_end(argp);
		m_category->error(ch);
	}
}

void Log::error(const std::string& message)
{
	if(m_category)
	{
		m_category->error(message);
	}
}

void Log::warn(const char* stringFormat, ...)
{
	if(m_category)
	{
		va_list argp;
		char ch[FILE_CONENT_BUFFER_MAX];
		va_start(argp, stringFormat);
		vsprintf(ch, stringFormat, argp);
		va_end(argp);
		m_category->warn(ch);
	}
}

void Log::warn(const std::string& message)
{
	if(m_category)
	{
		m_category->warn(message);
	}
}

void Log::notice(const char* stringFormat, ...)
{
	if(m_category)
	{
		va_list argp;
		char ch[FILE_CONENT_BUFFER_MAX];
		va_start(argp, stringFormat);
		vsprintf(ch, stringFormat, argp);
		va_end(argp);
		m_category->notice(ch);
	}
}

void Log::notice(const std::string& message)
{
	if(m_category)
	{
		m_category->notice(message);
	}
}

void Log::info(const char* stringFormat, ...)
{
	if(m_category)
	{
		va_list argp;
		char ch[FILE_CONENT_BUFFER_MAX];
		va_start(argp, stringFormat);
		vsprintf(ch, stringFormat, argp);
		va_end(argp);
		m_category->info(ch);
	}
}

void Log::info(const std::string& message)
{
	if(m_category)
	{
		m_category->info(message);
	}
}

void Log::debug(const char* stringFormat, ...)
{
	if(m_category)
	{
		va_list argp;
		char ch[FILE_CONENT_BUFFER_MAX];
		va_start(argp, stringFormat);
		vsprintf(ch, stringFormat, argp);
		va_end(argp);
		m_category->debug(ch);
	}
}

void Log::debug(const std::string& message)
{
	if(m_category)
	{
		m_category->debug(message);
	}
}
