#include    <iostream>
#include    <unistd.h>

#include    "udp_server.h"
#include    "t_PortPools.h"
#include    "t_pthread_manager.h"

#include    <sys/types.h>          /* See NOTES */
#include    <sys/socket.h>
#include    <strings.h>

MtsUdp::MtsUdp(const std::string& sIp, unsigned short usPort)
    :m_sIp(sIp), m_usPort(usPort), m_iSock(-1), m_pEvent(NULL),
    m_pEventBase(NULL)
{
}

MtsUdp::~MtsUdp()
{
    if (m_iSock >0)
    {
        ::close(m_iSock);
        m_iSock = -1;
    }
    if (m_pEvent)
    {
        event_del(m_pEvent);
        event_free(m_pEvent);
        m_pEvent = NULL;
    }
    if (m_pEventBase)
    {
        event_base_free(m_pEventBase);
        m_pEventBase = NULL;
    }
}

void MtsUdp::UdpThreadCallback()
{
    m_pEventBase = event_base_new();
    MTS_LOG_INFO("udp server thread, udp main event base: %p", m_pEventBase);

    bool bRet = this->Init();
    if (bRet == false)
    {
        MTS_LOG_ERROR("udp server init() fail, udp server thread exit");
        event_base_free(m_pEventBase);
        m_pEventBase = NULL;
        return ;
        //exit(0);
    }

    bRet = this->Run();
    if (bRet == false)
    {
        MTS_LOG_ERROR("udp server run() fail");
    }

    event_base_free(m_pEventBase);
    m_pEventBase = NULL;
}

bool MtsUdp::Init()
{
    m_iSock = ::socket( AF_INET,  SOCK_DGRAM | SOCK_NONBLOCK, 0 );
    if ( m_iSock <= 0 )
    {
        MTS_LOG_ERROR("create udp server main sock fail, err msg: %s", strerror(errno));
        return false;
    }
    MTS_LOG_DEBUG("udp srv fd: %d", m_iSock);

    int onePortMapServerCount           = 1 ;
    int RECV_BUF_SIZE                   =  1024 * 1024 * 64 ; //
    int SEND_BUF_SIZE                   =  1024 * 1024 *  4 ; //
    struct timeval udpRecvTimeOut       = {1,0};    //1s
    struct timeval udpSendTimeOut       = {1,0};    //1s

    int rc = ::setsockopt(m_iSock, SOL_SOCKET, SO_REUSEADDR, &onePortMapServerCount, sizeof(int));// 同一个端口绑定服务器数量
    if ( rc < 0)
    {
        MTS_LOG_ERROR("setsockopt onePortMapServerCount failed: %d, errMsg: %s",
                      errno, strerror(errno));
        return false;
    }

    rc = ::setsockopt(m_iSock, SOL_SOCKET, SO_RCVTIMEO,  &udpRecvTimeOut, sizeof(udpRecvTimeOut)); // 发送时限

    rc = ::setsockopt(m_iSock, SOL_SOCKET, SO_SNDTIMEO,  & udpSendTimeOut, sizeof(udpSendTimeOut));// 接收时限
    rc = ::setsockopt(m_iSock,  SOL_SOCKET,  SO_RCVBUF,  & RECV_BUF_SIZE,  sizeof( RECV_BUF_SIZE ) ); // 接收缓冲区
    if ( rc != 0 )
    {
        MTS_LOG_ERROR("setsockopt udpRecvBuffer failed: %d!", errno);
    }

    rc = ::setsockopt(m_iSock, SOL_SOCKET, SO_SNDBUF, &SEND_BUF_SIZE,  sizeof( SEND_BUF_SIZE ) ); // 发送缓冲区
    if ( rc != 0 )
    {
        MTS_LOG_ERROR("setsockopt udpSendBuffer failed: %d!", errno);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family   = AF_INET;
    serverAddr.sin_port     = ::htons( m_usPort ) ;

    if ( m_sIp.empty() )
    {
        serverAddr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        serverAddr.sin_addr.s_addr = ::inet_addr(const_cast<char*>(m_sIp.c_str()));
    }

    ::bzero( &serverAddr.sin_zero, sizeof(serverAddr.sin_zero) );
    rc = ::bind( m_iSock, (sockaddr*)&serverAddr, sizeof(serverAddr) );
    MTS_LOG_INFO( "udp server: bind() ret: %d", rc );

    if ( rc != 0 )
    {
        MTS_LOG_ERROR( "bind udp sockaddr failed! port: %d, ip: %s, fd: %d, err msg: %s", 
                      m_usPort, m_sIp.c_str(), m_iSock, strerror(errno) );
        return false;
    }
    return true;
}

void MtsUdp::EventCallback(int fd, short event, void *arg)
{
    struct sockaddr_in addr;
    socklen_t size = sizeof(addr);
    
    int iRecvLen = sizeof(RelayPortRecord);
    char buf[iRecvLen] = { 0 };
   
    int len = ::recvfrom( fd,  buf,  sizeof( buf ), 0, (struct sockaddr *)&addr, &size );
    
    if (len < 0)
    {
        MTS_LOG_ERROR("udp recvfrom err, errmsg: %s", strerror(errno));
        return ;
    }
    if (iRecvLen != len)
    {
        MTS_LOG_ERROR("recvfrom data len != really data len");
        return ;
    }
    PthreadManager::Instance()->DispatchUdpMsg( buf, len );
}

bool MtsUdp::Run()
{
    m_pEvent = event_new( m_pEventBase, m_iSock,  EV_READ | EV_PERSIST,  EventCallback,  this );
    if (m_pEvent == NULL)
    {
        MTS_LOG_ERROR("event_new() for udp server event fail");
        return false;
    }

    int iRet = event_add(m_pEvent, NULL);
    if (iRet < 0)
    {
        MTS_LOG_ERROR("event_add() for udp server event fail");
        exit(0);
    }
    MTS_LOG_INFO("udp server thread create and start ok");

    event_base_dispatch(m_pEventBase);
    return true;
}
