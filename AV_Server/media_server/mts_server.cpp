#include "mts_server.h"
#include "udp_server.h"
#include "t_pthread_manager.h"

#include "t_eventlisten.hpp"
#include "t_eventconn.hpp"
#include "t_report_load.h"
#include "t_timer_report.h"
#include "t_PortPools.h"
#include "t_socket.hpp"
#include "t_busi_module.h"
#include "proto_inner.h"
#include "t_udp_client.h"
#include "t_busi_interface.h"
#include "avs_mts_log.h"
#include "t_udp_client.h"

#include <unistd.h>
#include <sstream>

MtsTcp::MtsTcp(const std::string& sIp, unsigned int uiPort)
    : m_sSrvIp(sIp), m_uiSrvPort(uiPort), m_sSrvInternetIp(""), 
    m_uiSrvInternetPort(0), m_bInit(false),
    m_pListenSock(NULL), m_pEventBase(NULL), m_pListenConn(NULL),
    m_pPortPool(NULL), m_iCountAccept(0), m_sPolicyIp(""),
    m_usPolicyPort(0), m_pTimerRport(NULL)
{
    try {
       m_bInit = Init(); 
    }
    catch (...)
    {
        m_bInit = false;
        return ;
    }
}

MtsTcp::~MtsTcp()
{
    if (m_pListenSock) 
    {
        delete m_pListenSock;
        m_pListenSock = NULL;
    }
    if (m_pListenConn)
    {
        delete m_pListenConn;
        m_pListenConn = NULL;
    }
    for(auto& clientConn: m_mpClientConn)
    {
        delete clientConn.second;
    }
    m_mpClientConn.clear();

    if (m_pPortPool)
    {
        delete m_pPortPool;
        m_pPortPool = NULL;
    }
    if (m_pTimerRport)
    {
        delete m_pTimerRport;
        m_pTimerRport = NULL;
    }
    //add other del resource...
    if (m_pLoadReport)
    {
        delete m_pLoadReport;
        m_pLoadReport = NULL;
    }

    //last del step...
    if (m_pEventBase)
    {
        event_base_free(m_pEventBase);
        m_pEventBase = NULL;
    }
}

void CallBackReportTimer(int fd, short event, void *arg)
{
    MTS_LOG_DEBUG("report load timer on...");
    MtsTcp* pMtsTcp = (MtsTcp*)arg;
    if (!pMtsTcp) 
    {
        return ;
    }

    if ( false == pMtsTcp->LoadRport() )
    {
        MTS_LOG_ERROR("load report this time fail");
    }
}

bool MtsTcp::Init()
{
    if (!m_pListenSock)
    {
        m_pListenSock = new Sock(m_sSrvIp, m_uiSrvPort);
    }

    bool bRet = m_pListenSock->Listen();
    if (bRet == false)
    {
        MTS_LOG_INFO("tcp server listen fail on ip: %s, port: %d", 
                     m_sSrvIp.c_str(), m_uiSrvPort);
        return false;
    }

    struct event_config *ev_config = event_config_new();
    event_config_set_flag(ev_config, EVENT_BASE_FLAG_NOLOCK);
    m_pEventBase = event_base_new_with_config(ev_config);
    event_config_free(ev_config); 
    
    InitBusiModulePool();

    if (m_pPortPool == NULL)
    {
        m_pPortPool = new PortPool();
    }
    unsigned short udpPortStart = ::atoi(ConfigXml::Instance()->getValue("PortPool", "udpPortStart").c_str());
    unsigned short udpPortCount = ::atoi(ConfigXml::Instance()->getValue("PortPool", "udpPortCount").c_str());
    
    if ( udpPortStart <= 0 || udpPortCount <= 0 )
    {
        MTS_LOG_ERROR( "get PortPool->udpPortStart: %d,PortPool->udpPortCount: %d", 
                      udpPortStart,udpPortCount );
        return false;
    }

    m_pPortPool->InitPortPool(udpPortStart,udpPortCount);

    m_sUdpSrvIp =  ConfigXml::Instance()->getValue("UdpServer", "IP");
    m_usUdpPort =  ::atoi(ConfigXml::Instance()->getValue("UdpServer", "Port").c_str());

    if (m_sUdpSrvIp.empty() || m_usUdpPort <= 0)
    {
        MTS_LOG_ERROR("get udp srv ip or port fail from config, udp ip: %s, udp listen port: %d",
                      m_sUdpSrvIp.c_str(), m_usUdpPort);
        return false;
    }
    MTS_LOG_INFO("read conf, mts udp srv ip: %s, port: %d", m_sUdpSrvIp.c_str(), m_usUdpPort);

    m_pUdpClient.reset(new UdpClient(m_sUdpSrvIp.c_str(), m_usUdpPort));

    m_sPolicyIp     = ConfigXml::Instance()->getValue("MediaServer", "PolicyIP");
    m_usPolicyPort  = ::atoi(ConfigXml::Instance()->getValue("MediaServer","PolicyPort").c_str());
    MTS_LOG_INFO("from conf, policy srv ip: %s, port: %d", m_sPolicyIp.c_str(), m_usPolicyPort);

    int iReportTmDef = ::atoi(ConfigXml::Instance()->getValue("MediaServer","ReportTm").c_str());
    if (iReportTmDef <= 0)
    {
        iReportTmDef = 5; //unit is second;
    }
    MTS_LOG_DEBUG("avs mts report load timer tm: %ds", iReportTmDef);

    m_pTimerRport = new TimerReport(m_pEventBase, iReportTmDef, CallBackReportTimer, this);
    MTS_LOG_INFO("new report load timer obj");

    m_pLoadReport = new LoadReport(this);

    MTS_LOG_INFO("avs mts init succ, main loop base event addr: %p", m_pEventBase);
    m_bInit = true;
    return m_bInit;
}

bool MtsTcp::RegisterAcceptConn()
{
    m_pListenConn = new ListenConn(m_pListenSock->GetSockFd(),(void*)this);
    if (!m_pListenConn)
    {
        MTS_LOG_ERROR("new listen conn fail");
        return false;
    }

    bool bRet = m_pListenConn->AddEvent(m_pEventBase, EV_READ|EV_PERSIST, this);
    if (bRet == false)
    {
        MTS_LOG_ERROR("add read and persist event on listen fd: %d", 
                      m_pListenSock->GetSockFd());
        return false;
    }
    MTS_LOG_INFO("registe read and persist event succ, main listen fd: %d",
                 m_pListenSock->GetSockFd());
    return true;
}

bool MtsTcp::Run()
{
    if (m_bInit == false)
    {
        MTS_LOG_ERROR("init fail, check init process");
        return false;
    }

    if (RegisterAcceptConn() == false)
    {
        MTS_LOG_ERROR("set listen fd to accept on fd: %d fail", 
                      m_pListenSock->GetSockFd()); 
        return false;
    }

    //create and start udp server thread instance 
    MtsUdp  udpSrv(m_sUdpSrvIp, m_usUdpPort); 
    std::thread udpThread(&MtsUdp::UdpThreadCallback, &udpSrv);
    usleep(100);
    MTS_LOG_INFO("start new udp server thread done");

    int iDefThreadNum = 
        ::atoi(ConfigXml::Instance()->getValue("MediaServer", "ThreadNumber").c_str());
    if (iDefThreadNum <= 0)
    {
        iDefThreadNum = 10;
    }
    MTS_LOG_INFO("mediaserver work thread nums: %d", iDefThreadNum);

    PthreadManager::Instance()->Init( iDefThreadNum );

    //main event loop.......
    int iRet = event_base_loop(m_pEventBase, 0);
    if ( iRet != 0 )
    {
        MTS_LOG_INFO("tcp main server exit !!!");
        return true;
    }

    udpThread.join();
    return true;
}

bool MtsTcp::Accept()
{
    struct sockaddr_in clientAddr;
    int iAcceptFd = m_pListenSock->SockAccept((struct sockaddr*)&clientAddr);
    if (iAcceptFd <= 0)
    {
        MTS_LOG_ERROR("accept new conn fail on fd: %d", m_pListenSock->GetSockFd());
        return false;
    }
    MTS_LOG_DEBUG("accept new conn, new conn fd: %d, client ip: %s", 
                  iAcceptFd, ::inet_ntoa(clientAddr.sin_addr));

    AcceptConn* pNewConn = new AcceptConn(iAcceptFd, this);
    if (!pNewConn)
    {
        MTS_LOG_ERROR("new client conn obj fail on fd: %d", iAcceptFd);
        return false;
    }

    AddNewConnReadEvent(pNewConn);

    m_mpClientConn[iAcceptFd] = pNewConn;
    m_iCountAccept++;
    
    return true;
}

bool MtsTcp::AddNewConnReadEvent(AcceptConn* pAcceptConn)
{
    return pAcceptConn->AddEvent(m_pEventBase, EV_READ | EV_PERSIST, this);
}

bool MtsTcp::DeleteAcceptConn(int iFd)
{
    if (iFd <= 0)
    {
        return false;
    }
    
    auto it = m_mpClientConn.find(iFd);
    if (it != m_mpClientConn.end())
    {
        delete it->second;
        m_mpClientConn.erase(it);
        m_iCountAccept--;
    }
    return true;
}

void MtsTcp::InitBusiModulePool()
{
    m_mpBusiModule.clear();

    m_mpBusiModule[CMD_NO_ALLOCATE_RELAY_PORT_REQ] 
        = std::make_shared<OpenChannel>(OpenChannel(CMD_NO_ALLOCATE_RELAY_PORT_REQ, this));

    m_mpBusiModule[CMD_NO_RELEASE_RELAY_PORT_REQ] 
        = std::make_shared<ReleaseChannel>(ReleaseChannel(CMD_NO_RELEASE_RELAY_PORT_REQ, this));

    MTS_LOG_DEBUG("load busi process module succ! module nums: %d, busi module: %s, %s",
                  m_mpBusiModule.size(), CMD_NO_ALLOCATE_RELAY_PORT_REQ,
                  CMD_NO_RELEASE_RELAY_PORT_REQ);
}


bool MtsTcp::SendToUdpSrvCmd(const std::string& sIp, unsigned short usPort,
                             const void* pData, unsigned short iDataLen)
{
    return m_pUdpClient->sendTo(sIp, usPort, pData, iDataLen);
}

bool MtsTcp::LoadRport()
{
    bool bRet = false;
    rapidjson::Document oRoot;
    if (BuildLoadPack(oRoot) == false)
    {
        MTS_LOG_ERROR("build mts load msg fail");
        return false;
    }

    bRet = m_pLoadReport->Send(m_sPolicyIp, m_usPolicyPort, oRoot);
    if (bRet == false)
    {
        MTS_LOG_ERROR("send load to policy fail, policy ip: %s, port: %d", 
                      m_sPolicyIp.c_str(), m_usPolicyPort);
        return false;
    }
    return true;
}

bool MtsTcp::BuildLoadPack(rapidjson::Document& oRoot)
{
    oRoot.SetObject();
    rapidjson::Document::AllocatorType& allocator =
        oRoot.GetAllocator();

    rapidjson::Value author;
    
    std::string sReportCmd(CMD_NO_REGISTER_MTS_REQ);
    author.SetString(sReportCmd.c_str(), sReportCmd.size(), allocator);
    oRoot.AddMember("method", author, allocator);

    static int iReq_id(100);
    oRoot.AddMember("req_id", iReq_id, allocator);
    iReq_id++;

    //TODO:
    std::string sLocalTraceId = std::to_string(time(NULL)); 
    author.SetString(sLocalTraceId.c_str(), sLocalTraceId.size(), allocator);
    oRoot.AddMember("traceId", author, allocator);

    //TODO:
    std::string sSpanId = std::to_string(time(NULL));
    author.SetString(sSpanId.c_str(), sSpanId.size(), allocator);
    oRoot.AddMember("spanId", author, allocator);

    int iTimeStamp = time(NULL);
    oRoot.AddMember("timestamp", iTimeStamp, allocator);


    rapidjson::Value jsLoad(rapidjson::kObjectType);
    jsLoad.AddMember("conn_nums", m_iCountAccept.load(), allocator);
   
    std::string reportLocalIp; 
    unsigned int uiPort;
    if ( m_sSrvInternetIp.empty() ) 
    {
        reportLocalIp = m_sSrvIp;
        MTS_LOG_DEBUG("not config local internet ip, use intranet ip: %s", m_sSrvIp.c_str());
    } 
    else
    {
        reportLocalIp = m_sSrvInternetIp;
    }
    if ( m_uiSrvInternetPort <= 0 )
    {
        MTS_LOG_DEBUG("not config local internet port, use intranet port: %d", m_uiSrvPort);
        uiPort = m_uiSrvPort;
    }
    else 
    {
        uiPort = m_uiSrvInternetPort;
    }
    rapidjson::Value jsParams(rapidjson::kObjectType);
    author.SetString(reportLocalIp.c_str(), reportLocalIp.size(), allocator);
    jsParams.AddMember("ip", author, allocator);
    jsParams.AddMember("port", uiPort, allocator);
    jsParams.AddMember("load", jsLoad, allocator);

    oRoot.AddMember("params", jsParams, allocator);
    
    StringBuffer buffer; 
    Writer<StringBuffer> writer(buffer);
    oRoot.Accept(writer);
    
    std::string sPkg = buffer.GetString();
    MTS_LOG_DEBUG( "send pkg => %s", sPkg.c_str() );

    return true;
}


PortPool* MtsTcp::GetPortPool()
{
    return m_pPortPool;
}


void MtsTcp::SetLocalInternetIp(const std::string& sIp)
{
    m_sSrvInternetIp = sIp;
}

void MtsTcp::SetLocalInternetPort(unsigned int uiPort)
{
    m_uiSrvInternetPort = uiPort;
}
