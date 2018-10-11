#include <iostream>
#include "t_PortPools.h"
#include <string.h>
#include <algorithm>

PortPool::PortPool(): m_usUdpPortBase(0), m_usUdpPortCount(0)
{
    m_mpClientSession.clear();
    //m_vUdpPortPool.clear();
}

PortPool::~PortPool()
{
}

void PortPool::InitPortPool(unsigned short usUdpBase, unsigned short usUdpPortCount)
{
    m_usUdpPortBase     =   usUdpBase;
    m_usUdpPortCount    =   usUdpPortCount;
    
    for (int i = 0; i < usUdpPortCount; ++i)
    {
        m_pqPortPools.push( m_usUdpPortBase + i );
    }
}

#if 0
unsigned short PortPool::GetUdpPort()
{
    std::lock_guard<std::mutex> lock(m_udpPoolMutex);

    unsigned short tmpUdpPort = 0;
    if (m_vUdpPortPool.size() > 0)
    {
        tmpUdpPort = m_vUdpPortPool.front();
        m_vUdpPortPool.pop_front();
    }
    return tmpUdpPort;
}
#endif

bool PortPool::CheckPortUsable(unsigned short usPort)
{
    int listenfd;
    struct sockaddr_in sockaddr;
    ::memset(&sockaddr,0,sizeof(sockaddr));

    sockaddr.sin_family         = AF_INET;
    sockaddr.sin_addr.s_addr    = htonl(INADDR_ANY);
    sockaddr.sin_port           = htons(usPort);

    listenfd = socket(AF_INET,(SOCK_DGRAM),0);
    if (listenfd == -1)
    {
        return false;
    }

    int iRet = bind(listenfd,(struct sockaddr *) &sockaddr,sizeof(sockaddr));
    if(iRet == -1)
    {
        close(listenfd);
        return false;
    }

    close(listenfd);    
    return true;
}

#if 0
bool PortPool::CheckUdpPortPairsUsable(unsigned short usPort)
{
    return (CheckPortUsable(usPort) && CheckPortUsable(usPort+1));
}
#endif

bool PortPool::CheckUdpPortPairsUsable(std::pair<unsigned short,unsigned short>  usPort)
{
    return (CheckPortUsable(usPort.first) && CheckPortUsable(usPort.second));
}

#if 0
unsigned short PortPool::GetAndCheckUdpPort(int iRedo)
{
    unsigned short usPort = 0;
    do 
    {
        usPort = GetUdpPort();
        if (usPort == 0)
        {
            break;
        }
        if (CheckUdpPortPairsUsable(usPort))
        {
            break;
        }

        RecycleUdpPort(usPort);
        usPort = 0;
    } while (iRedo-- >0);
    return usPort;
}

void PortPool::RecycleUdpPort(unsigned short usPort)
{
    if (usPort > 0)
    {
        std::lock_guard<std::mutex> lock(m_udpPoolMutex);
        auto it = std::find( m_vUdpPortPool.begin(), m_vUdpPortPool.end(), usPort);
        if (it == m_vUdpPortPool.end())
        {
            m_vUdpPortPool.push_back(usPort);
        }
    }
}
#endif

void PortPool::RecycleUdpPort(std::pair<unsigned short,unsigned short>  usPort)
{
    std::lock_guard<std::mutex> lock(m_udpPoolMutex);
    if (usPort.first > 0)
    {
        m_pqPortPools.push(usPort.first);
    }
    if (usPort.second > 0)
    {
        m_pqPortPools.push(usPort.second);
    }
}

std::pair<unsigned short, unsigned short> 
PortPool::GetUdpRtpRtcpPort()
{
    std::pair<unsigned short, unsigned short> portRTP_RTCP;
    std::list<unsigned int> tmpPortList;
    
    std::lock_guard<std::mutex> lock(m_udpPoolMutex);
    tmpPortList.clear();
    portRTP_RTCP.first = portRTP_RTCP.second = 0;
    do {
        if ( m_pqPortPools.empty() || m_pqPortPools.size() < 2 )
        {
            for ( const auto& one: tmpPortList )
            {
                m_pqPortPools.push( one );
            }
            return portRTP_RTCP;
        }

        if ( (m_pqPortPools.top() % 2) == 0)
        {
            portRTP_RTCP.first = m_pqPortPools.top();
            m_pqPortPools.pop();
            
            portRTP_RTCP.second = m_pqPortPools.top();
            m_pqPortPools.pop();
            
            for ( const auto& one: tmpPortList )
            {
                m_pqPortPools.push( one );
            }
            return portRTP_RTCP;
        }

        tmpPortList.push_front( m_pqPortPools.top() );
        m_pqPortPools.pop();

    } while( !m_pqPortPools.empty() );

    return portRTP_RTCP;
}

std::pair<unsigned short, unsigned short>
PortPool::GetAndCheckUdpRtpRtcpPort(int iRedo)
{
    std::pair<unsigned short,unsigned short> portRtpRtcp;
    portRtpRtcp.first = portRtpRtcp.second = 0;

    do {
        portRtpRtcp = GetUdpRtpRtcpPort( );
        if ( portRtpRtcp.first == 0 || portRtpRtcp.second == 0 )
        {
            break;
        }
        if ( CheckUdpPortPairsUsable(portRtpRtcp) )
        {
            break;
        }

        RecycleUdpPort(portRtpRtcp);
        portRtpRtcp.first = portRtpRtcp.second = 0;

    } while( iRedo -- >0 );

    return portRtpRtcp;
}
