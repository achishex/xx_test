/**
 * @file: mts_server_main.cpp
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-07-31
 */

#include "avs_mts_log.h"
#include "mts_server.h"

#include "configXml.h"
#include "file_singleton.h"
#include "unix_util.h"
#include <memory>

#define avs_mts_srv_name        "avs_media_server"
#define avs_mts_conf_name       "../conf/avs_media_srv.xml"

Log *pAvsLog;

class AvsMtsInstance
{
 public:
  AvsMtsInstance(int argc, char* argv[]);
  virtual ~AvsMtsInstance();
  void Run();

 private:
  bool Init();
  void Usage();
  void SetMainOpt();
  static void HandleKillSignal(int sig);
 private:
  int m_iArgc;
  char** m_pArgv;
  bool m_bInit;
  std::string m_sIp;
  unsigned int m_uiPort;
  unsigned int m_uiMaxConnNums;
};

//---------------------------------------------//
AvsMtsInstance::AvsMtsInstance(int argc, char* argv[])
    :m_iArgc(argc), m_pArgv(argv), m_bInit(false), 
    m_sIp("0.0.0.0"), m_uiPort(6544), m_uiMaxConnNums(20000)
{
}

AvsMtsInstance::~AvsMtsInstance()
{
}

bool AvsMtsInstance::Init()
{
    setbuf(stderr, NULL);
#ifdef SINGLE_AVS_MTS_NODE
    if (0 != avs_util::singleton("/var/tmp/avs_mts.lock"))
#else 
    if (0 != avs_util::singleton("./avs_mts.lock"))
#endif
    {
        return false;
    }

    if (m_iArgc != 2)
    {
        Usage();
        return false;
    }

    int iRet = ConfigXml::Instance()->init(m_pArgv[1]);
    if (iRet != 0)
    {
        std::cout << "read conf: [  " << m_pArgv[1] << " ] fail" << std::endl;
        Usage();
        return false;
    }
    string filePath = ConfigXml::Instance()->getValue("Log", "logPropertiesFilePath");
    string fileName = ConfigXml::Instance()->getValue("Log", "logPropertiesFileName");
    string conf_file_path = filePath + fileName; 
    pAvsLog = new Log(conf_file_path);

    MTS_LOG_INFO("the program %s is starting........................", m_pArgv[0]);
    SetMainOpt();

    m_sIp = ConfigXml::Instance()->getValue("MediaServer","IP");
    m_uiPort = ::atoi( ConfigXml::Instance()->getValue("MediaServer", "Port" ).c_str());
    if (m_sIp.empty() || m_uiPort <= 0)
    {
        MTS_LOG_ERROR("read srv ip or port err, ip: %s, port: %d", m_sIp.c_str(), m_uiPort);
        return false;
    }
    //...

    m_bInit = true;
    return true;
}

void AvsMtsInstance::SetMainOpt()
{
    std::shared_ptr<Util::ProcTitle> PTile(new Util::ProcTitle(m_iArgc, m_pArgv));
    char buf[512] = {0};
    getcwd(buf, sizeof(buf));
    snprintf(buf+strlen(buf),sizeof(avs_mts_srv_name) + 1,"/%s", (avs_mts_srv_name));
    PTile->SetTitle(buf);
#ifndef no_daemon 
    Util::daemonize(PTile->GetTitle());
#endif
    Util::InstallSignal(AvsMtsInstance::HandleKillSignal);
	Util::SetOpenFileNums(m_uiMaxConnNums);
}

void AvsMtsInstance::HandleKillSignal(int sig)
{
    std::ostringstream  mtsOs;
    mtsOs << "Signal handled: " << strsignal(sig);
    std::string strProcessExistMsg;

    if (sig == SIGTERM)
    {   
        mtsOs << ", User Kill this Pid: " << getpid();
    }   
    else 
    {   

    }   
    mtsOs << ". now process manually exit.";
    strProcessExistMsg.append(mtsOs.str());
    MTS_LOG_ERROR("%s", strProcessExistMsg.c_str());

    exit(EXIT_SUCCESS);
}

void AvsMtsInstance::Run()
{
    if (Init() == false)
    {
        MTS_LOG_ERROR("srv: %s init fail !!!", avs_mts_srv_name);
        return ;
    }

    std::string sLocalInternetIp = ConfigXml::Instance()->getValue("MediaServer","InternetIP");
    unsigned int uiLocalInternetPort = ::atoi( ConfigXml::Instance()->getValue("MediaServer", "InternetPort" ).c_str());
    MtsTcp mtsTcpInstance(m_sIp, m_uiPort);
    
    mtsTcpInstance.SetLocalInternetIp(sLocalInternetIp);
    mtsTcpInstance.SetLocalInternetPort(uiLocalInternetPort);

    if (false == mtsTcpInstance.Run())
    {
        MTS_LOG_ERROR("%s tcp run fail", avs_mts_srv_name);
        return ;
    }
    else 
    {
        MTS_LOG_INFO("routine: %s exit", avs_mts_srv_name);
    }
    return ;
}

void AvsMtsInstance::Usage()
{
    fprintf(stdout, "Usage: \n\r    %s  %s \n\r", avs_mts_srv_name, avs_mts_conf_name);
    return ;
}

int main(int argc, char* argv[])
{
    AvsMtsInstance avs_mts(argc, argv);
    avs_mts.Run();
    return 0;
}
