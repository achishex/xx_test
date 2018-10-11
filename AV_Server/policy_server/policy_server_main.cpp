/**
 * @file: t_main.cpp
 * @brief: 
 *       tcp 服务的入口
 * @author: 
 * @version: v0x0001
 * @date: 2018-04-24
 */

#include <iostream>
#include <stdlib.h>
 #include <stdio.h>

#include "policy_server.hpp"
#include "configXml.h"
#include "file_singleton.h"
#include "policy_log.h"
#include "unix_util.h"

using namespace std;
using namespace T_TCP;

#define Policy_srv_conf_name    "../conf/avs_policy_srv.xml"
#define Policy_srv_name         "policy_server"

Log *pAvsLog; 

class PolicySrvInstance
{
 public:
  PolicySrvInstance(int argc, char** argv);
  virtual ~PolicySrvInstance();
 
  void Run();

 private:
  bool Init();
  void Usage();
  void SetMainOpt();
  static void HandleKillSignal( int sig );
 private:
  bool bInit;
  int m_iArgc;
  char** m_pArgv;
  std::string m_sIp;
  int m_uiPort;
  int m_uiThreadNums;
};

//---------------------- implement ----------------------------//
PolicySrvInstance::PolicySrvInstance(int argc, char** argv)
    : bInit(false), m_iArgc(argc), m_pArgv(const_cast<char**>(argv)),m_sIp("0.0.0.0"),
    m_uiPort(6543), m_uiThreadNums(10)
{
}

PolicySrvInstance::~PolicySrvInstance()
{
}


bool PolicySrvInstance::Init()
{
    setbuf(stderr, NULL);
#ifdef SINGLE_POLICY_NODE
    if (0 != avs_util::singleton("/var/tmp/policy_server.lock"))
#else
        if (0 != avs_util::singleton("./policy_server.lock"))
#endif
     {
        return false;
     }

    if (m_iArgc < 2)
    {
        Usage();
        return false;
    }

    int iRet = ConfigXml::Instance()->init(m_pArgv[1]);
    if (iRet != 0)
    {
        std::cerr << "read cnf: " << m_pArgv[1] << " fail" << std::endl;
        return false;
    }

    string filePath = ConfigXml::Instance()->getValue("Log", "logPropertiesFilePath");                                                                                             
    string fileName = ConfigXml::Instance()->getValue("Log", "logPropertiesFileName");                                                                                             
    string conf_file_path = filePath + fileName;                                                                                                                                   
    pAvsLog = new Log(conf_file_path); 
    
    PL_LOG_INFO("the program %s is starting >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>", m_pArgv[0]);
    SetMainOpt();

    m_sIp = ConfigXml::Instance()->getValue("PolicyServer", "IP");
    m_uiPort = ::atoi(ConfigXml::Instance()->getValue("PolicyServer", "Port").c_str());
    if (m_sIp.empty() || m_uiPort <= 0)
    {
        PL_LOG_ERROR("read srv ip or port err, ip: %s, port: %d", m_sIp.c_str(), m_uiPort);
        return false;
    }

    int uiThreadNumsDef = m_uiThreadNums;
    m_uiThreadNums = ::atoi(ConfigXml::Instance()->getValue("PolicyServer", "Threadnums").c_str());
    if (m_uiThreadNums <= 0)
    {
        PL_LOG_NOTICE("main thread nums: %d read from conf fail, use default num: %d",
                      m_uiThreadNums, uiThreadNumsDef);
        m_uiThreadNums = uiThreadNumsDef;
    }

    PL_LOG_DEBUG("item from cnf, local ip: %s, local port: %d, work thread nums: %d",
                 m_sIp.c_str(),m_uiPort, m_uiThreadNums);
    bInit = true;
    return true;
}

void PolicySrvInstance::Usage()
{
    fprintf(stdout, "Usage:\n\r     %s  %s \n\r", Policy_srv_name, Policy_srv_conf_name);
    return ;
}

void PolicySrvInstance::Run()
{
    if (Init() == false)
    {
        PL_LOG_ERROR(" srv: %s init fail !!!", Policy_srv_name);
        return ;
    }
    PL_LOG_DEBUG("%s Init() succ", Policy_srv_name);

    TcpSrv tcpSrv(m_sIp, m_uiPort, m_uiThreadNums);
    if (tcpSrv.Run() == false) 
    {
        PL_LOG_ERROR("%s run fail",  Policy_srv_name);
    } 
    else 
    {
        PL_LOG_INFO("routine: %s  exit", Policy_srv_name);
    }

    return ;
}

void PolicySrvInstance::SetMainOpt()
{
    std::shared_ptr<Util::ProcTitle> PTile(new Util::ProcTitle(m_iArgc, m_pArgv));
    char buf[512] = {0};
    getcwd(buf, sizeof(buf));
    snprintf(buf+strlen(buf),sizeof(Policy_srv_name) + 1,"/%s", (Policy_srv_name));
    PTile->SetTitle(buf);

#ifndef no_daemon
    Util::daemonize(PTile->GetTitle());
#endif

    Util::InstallSignal(PolicySrvInstance::HandleKillSignal);

	Util::SetOpenFileNums(10000);
}

void PolicySrvInstance::HandleKillSignal(int sig)
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
    PL_LOG_INFO("%s", strProcessExistMsg.c_str());

    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    PolicySrvInstance policy_srv(argc, argv);
    policy_srv.Run();
    return 0;
}


