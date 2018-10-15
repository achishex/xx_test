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

bool PortPool::CheckUdpPortPairsUsable(std::pair<unsigned short,unsigned short>  usPort)
{
    return (CheckPortUsable(usPort.first) && CheckPortUsable(usPort.second));
}

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
            break;
        }

        if ((m_pqPortPools.top() % 2) == 0)
        {
            portRTP_RTCP.first = m_pqPortPools.top();
            m_pqPortPools.pop();
            
            portRTP_RTCP.second = m_pqPortPools.top();
            m_pqPortPools.pop();

            break;
        }

        tmpPortList.push_front( m_pqPortPools.top() );
        m_pqPortPools.pop();

    } while( !m_pqPortPools.empty() );

    for ( const auto& one: tmpPortList )
    {
        if ( one > 0 )
        {
            m_pqPortPools.push( one );
        }
    }
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
            RecycleUdpPort( portRtpRtcp );
            portRtpRtcp.first = portRtpRtcp.second = 0;
            continue;
        }
        if ( CheckUdpPortPairsUsable(portRtpRtcp) )
        {
            break;
        }

        RecycleUdpPort(portRtpRtcp);
        portRtpRtcp.first = portRtpRtcp.second = 0;

    } while( iRedo-- >0 );

    return portRtpRtcp;
}

unsigned int PortPool::GetUsablePortNums()
{
    std::lock_guard<std::mutex> lock(m_udpPoolMutex);
    return m_pqPortPools.size();
}

unsigned int PortPool::GetPortPoolCap() const
{
    return m_usUdpPortCount;
}
