#include "avs_mts_log.h"

#include "t_udp_session.h"
#include "t_udp_client.h"

#include <cstring>

#include <netinet/in.h> 
#include <arpa/inet.h>

///
SessionItem::~SessionItem()
{
    if (m_sock > 0)
    {
        event_del( &m_event );

        MTS_LOG_DEBUG("close fd: %d, session id: %s", m_sock, m_session_id.c_str());
        close(m_sock);
        m_sock = -1;
    }
    
    m_is_data_ok = false;
    m_session_id.clear();

    ::bzero(&m_remoteAddr, sizeof(m_remoteAddr)); 
}
///


UdpSession::UdpSession()
    : m_destroying(false)
{
}

UdpSession::~UdpSession()
{
    m_mpSessionPorts.clear();
    m_sessionId.clear();
}


bool UdpSession::Init( const RelayPortRecord& record, struct event_base * base )
{
    m_destroying        =   false;
    m_sessionId         =   record.m_sSessId;
    m_mpSessionPorts.clear();   
    
    int iCalllerARtpFd = CreateUdpEvent(record.m_sSessId, record.usCallerARtpPort, base);
    if (iCalllerARtpFd < 0)
    {
        return false;
    }

    int iCalllerARtcpFd = CreateUdpEvent(record.m_sSessId, record.usCallerARtcpPort, base);
    if (iCalllerARtcpFd < 0)
    {
        return false;
    }

    int iCallerVRtpFd = CreateUdpEvent(record.m_sSessId, record.usCallerVRtpPort, base);
    if (iCallerVRtpFd < 0)
    {
        return false;
    }

    int iCallerVRtcpFd = CreateUdpEvent(record.m_sSessId, record.usCallerVRtcpPort, base);
    if (iCallerVRtcpFd < 0)
    {
        return false;
    }

    int iCalleeARtpFd = CreateUdpEvent(record.m_sSessId, record.usCalleeARtpPort, base);
    if (iCalleeARtpFd < 0)
    {
        return false;
    }

    int iCalleeARtcpFd = CreateUdpEvent(record.m_sSessId, record.usCalleeARtcpPort, base);
    if (iCalleeARtpFd < 0)
    {
        return false;
    }
    
    int iCalleeVRtpFd = CreateUdpEvent(record.m_sSessId, record.usCalleeVRtpPort, base);
    if (iCalleeVRtpFd < 0)
    {
        return false;
    }

    int iCalleeVRtcpFd = CreateUdpEvent(record.m_sSessId, record.usCalleeVRtcpPort, base);
    if (iCalleeVRtcpFd < 0)
    {
        return false;
    }

    UpdatePeerFd(iCalllerARtpFd,    iCalleeARtpFd);
    UpdatePeerFd(iCalllerARtcpFd,   iCalleeARtcpFd);
    UpdatePeerFd(iCallerVRtpFd,     iCalleeVRtpFd);
    UpdatePeerFd(iCallerVRtcpFd,    iCalleeVRtcpFd);

    return true;
}

bool UdpSession::UpdatePeerFd(int irstFd, int indFd)
{
    auto it = m_mpSessionPorts.find(irstFd);
    if (it == m_mpSessionPorts.end())
    {
        MTS_LOG_ERROR("not find fd: %d in session map", irstFd);
        return false;
    }
    it->second->m_peer_sock = indFd;

    it = m_mpSessionPorts.find(indFd);
    if (it == m_mpSessionPorts.end())
    {
        MTS_LOG_ERROR("not find fd: %d in session map", indFd);
        return false;
    }
    it->second->m_peer_sock = irstFd;
    
    MTS_LOG_DEBUG("update fd pair: [%d, %d], session id: %s",
                  irstFd, indFd,  it->second->m_session_id.c_str());
    return true;
}

int UdpSession::CreateUdpEvent(const std::string& sessid, 
                               unsigned short usPort,
                               struct event_base* base)
{
    std::shared_ptr<SessionItem> sessionRecord(new SessionItem());

    int iSock = ::socket( AF_INET,  SOCK_DGRAM | SOCK_NONBLOCK,  0 );
    if ( iSock< 0 )
    {
        MTS_LOG_ERROR("create udp socket failed, port:%d, session id: %s",  
                      usPort, sessid.c_str());
        return -1;
    }


    static int onePortMapServerCount    = 1;
    static int RECV_BUF_SIZE            = 1024 * 1024 * 10;
    static int SEND_BUF_SIZE            = RECV_BUF_SIZE * 6;
    
    int ret = setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, 
                         &onePortMapServerCount, sizeof(int)); 
                         // 同一个端口绑定服务器数量
    if ( ret < 0)
    {
        close( iSock );
        iSock = -1 ;
        MTS_LOG_ERROR("setsockopt onePortMapServerCount failed: %d, session id:%s", 
                      errno, sessid.c_str());
        return -1;
    }

    ret = setsockopt(iSock, SOL_SOCKET,SO_RCVBUF, 
                     & RECV_BUF_SIZE, sizeof( RECV_BUF_SIZE ) );     // 接收缓冲区
    if ( ret )
    {
        MTS_LOG_ERROR( "setsockopt recv_buf_size=%d failed: %d, session id:%s",  
                      RECV_BUF_SIZE,  errno, sessid.c_str());
    }

    ret = setsockopt(iSock, SOL_SOCKET, SO_SNDBUF, 
                     &SEND_BUF_SIZE, sizeof( SEND_BUF_SIZE ) );    // 发送缓冲区
    if ( ret )
    {
        MTS_LOG_ERROR( "setsockopt send_buf_size=%d failed: %d, session id:%s", 
                      SEND_BUF_SIZE, errno, sessid.c_str());
    }

    struct sockaddr_in      serverAddr;
    bzero( &serverAddr, sizeof(serverAddr) );
    
    serverAddr.sin_family       = AF_INET;
    serverAddr.sin_port         = htons(usPort);
    serverAddr.sin_addr.s_addr  = INADDR_ANY;
   
    ret  =  ::bind( iSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if ( ret )
    {
        close( iSock );  
        iSock = -1;
        MTS_LOG_ERROR( "bind udp port=%d failed, session id:%s", usPort, sessid.c_str() );
        return -1;
    }

    event_assign( &sessionRecord->m_event, base, iSock, EV_READ | EV_PERSIST, recv_callback, this );

    ret  =  event_add( &sessionRecord->m_event, NULL );
    if ( ret  <  0 )
    {
        close( iSock );  
        iSock = -1 ;
        MTS_LOG_ERROR( "event_add failed! port: %d, session id:%s",  usPort, sessid.c_str());
        return -1;
    }

    sessionRecord->m_port       = usPort;
    sessionRecord->m_sock       = iSock;
    sessionRecord->m_is_data_ok = false;
    sessionRecord->m_session_id = sessid;

    m_mpSessionPorts[ iSock ] = sessionRecord;
    MTS_LOG_DEBUG("bind udp recv data fd: %d on  port: %d, session id: %s",
                  iSock, usPort, sessid.c_str());
    return iSock;
}

void UdpSession::CloseSession()
{
    m_mpSessionPorts.clear();
    MTS_LOG_DEBUG("Close session and recycle ports, session id: %s", m_sessionId.c_str());
    m_destroying = false ; //

    EventManager::Instance()->release( this ); //lock inside.
}

void UdpSession::recv_callback(int fd, short event, void* arg)
{
    UdpSession* e = reinterpret_cast< UdpSession* >( arg );
    e->ReadProcess( fd,  event );
}

void UdpSession::ReadProcess( int fd,  short event )
{
    struct sockaddr_in addr;
    socklen_t size = sizeof(addr);
    bzero( &addr,  size);

    char buf[ BUF_MAX_SIZE ] ;
    int recvLen = ::recvfrom( fd, buf, sizeof( buf ), 0, 
                             (struct sockaddr *)&addr,  
                             &size ); 
    auto it = m_mpSessionPorts.find(fd);
    if (it == m_mpSessionPorts.end())
    {
        MTS_LOG_ERROR("not find fd: %d in session", fd);
        return ;
    }

    if(recvLen < 0)
    {
        MTS_LOG_ERROR( "port:%d recv from %s err, errMsg: %s, sessionid:%s", 
                      it->second->m_port, inet_ntoa(addr.sin_addr), strerror(errno), 
                      it->second->m_session_id.c_str()); 
        return;
    }

    if (recvLen == 0 && m_destroying)
    {
        MTS_LOG_INFO("recv on port:%d, data src ip: %s, recv pkg len: %d, close conn, sessionid: %s", 
                     it->second->m_port, inet_ntoa(addr.sin_addr), recvLen,
                     it->second->m_session_id.c_str());
        this->CloseSession();
        return ;
    }

    if (it->second->m_is_data_ok == false)
    {
        MTS_LOG_INFO("first begin to recv data on fd: %d, port: %d, session id: %s",
                     it->second->m_sock, it->second->m_port, it->second->m_session_id.c_str());
        it->second->m_remoteAddr.sin_family  = AF_INET; 
        it->second->m_remoteAddr.sin_addr    = addr.sin_addr;
        it->second->m_remoteAddr.sin_port    = (addr.sin_port);
        it->second->m_is_data_ok = true;
    }

    int& iPeerFd    = it->second->m_peer_sock;
    auto itPeer     = m_mpSessionPorts.find(iPeerFd);
    if (itPeer == m_mpSessionPorts.end())
    {
        MTS_LOG_ERROR("recv peer data fd not been created");
        return ;
    }

    if (itPeer->second->m_is_data_ok == false)
    {
        MTS_LOG_DEBUG("peer node not send any data,omit it. local fd: %d, local port: %d"
                      ",peer local fd: %d, peer local port: %d, sessionid: %s",
                      it->second->m_sock, it->second->m_port, it->second->m_peer_sock,
                      itPeer->second->m_port, itPeer->second->m_session_id.c_str());
        return ;
    }

    if (m_destroying)
    {
        this->CloseSession();
        return ;
    }

    int reTryNums = 3;
    do {
        int iRetSend = ::sendto( itPeer->second->m_sock, buf, recvLen, 0, 
                                (struct sockaddr *)&(itPeer->second->m_remoteAddr),
                                sizeof(struct sockaddr_in) );
        if (iRetSend < 0)
        {
            if ( errno != EAGAIN && errno != EINTR )
            {
                MTS_LOG_ERROR( "send data to peer node fail,"
                               "session id: %s, err msg: %s",
                               itPeer->second->m_session_id.c_str(), 
                               strerror(errno) );
                return ;
            }
            break;
        }

        if (iRetSend != recvLen)
        {
            MTS_LOG_ERROR("send to peer node data len err");
            return ;
        }
        return ;

    } while(reTryNums-- > 0);

    if (reTryNums <= 0)
    {
        MTS_LOG_ERROR("retry send to data fail");
        return ;
    }
}

void UdpSession::NotifyCloseSession( )
{
    m_destroying    = true;
    auto sessionId  = m_mpSessionPorts.begin();
    if (sessionId == m_mpSessionPorts.end())
    {
        return ;
    }

    UdpClient notifyClient;
    char buf[10] = {0};

    MTS_LOG_DEBUG("send close  msg to session thread on port: %d,"
                  "session %s", sessionId->second->m_port, this->ToString().c_str());
    notifyClient.sendTo("0.0.0.0", sessionId->second->m_port, buf, 0);
}

std::string UdpSession::ToString()
{
    std::stringstream ios;
    for (const auto sess: m_mpSessionPorts)
    {
        ios << sess.second->ToString() << ",";
    }
    return ios.str();
}

/*-------------------------------------------------------------*/
UdpSession*  EventManager::acquire()
{
    UdpSession* e = NULL ;
    {
        std::lock_guard<std::mutex> lock(m_lock);
        if( !m_list.empty() )
        {
            e  =  * m_list.begin() ;        
            m_list.pop_front();
        }
    }

    if( e  ==  NULL )
    {
        e =  new UdpSession() ;
    }
    return e;
}

void EventManager::release(UdpSession* e )
{
    if( e  ==  NULL ) 
        return ;

    std::lock_guard<std::mutex> lock(m_lock);
    m_list.push_back( e ) ;
}

EventManager::EventManager():  m_base(NULL)
{
}

void EventManager::Init(event_base * base)
{
	m_base = base;
}
