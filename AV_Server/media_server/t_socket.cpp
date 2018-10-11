#include "t_socket.hpp"
#include "avs_mts_log.h"
#include <string.h>
#include <errno.h>


Sock::Sock(const std::string& sIp,  int iPort)
    :m_sSrvIp(sIp), m_iSrvPort(iPort), m_iFd(-1), m_err(-1)
{
    CreateSock();
}

Sock::~Sock()
{
    if (m_iFd > 0)
    {
        close(m_iFd);
        m_iFd = -1;
    }
}

int Sock::CreateSock()
{
    m_iFd = ::socket( AF_INET, SOCK_STREAM, 0 ); 
    if (m_iFd <= 0)
    {
        SetErr(SOCK_ERR);
        MTS_LOG_ERROR("create loacal sock fail");
        return -1;
    }
    MTS_LOG_DEBUG("create sock fd: %d", m_iFd);

    if (false == SetSockBlock(m_iFd, false))
    {
        MTS_LOG_ERROR("set sock O_NONBLOCK on fd: %d fail, err msg: %s", 
                      m_iFd, strerror(errno));

        SetErr(SOCK_ERR);
        ::close(m_iFd);
        m_iFd = -1;
        return -2;

    }
    return m_iFd;
}

bool Sock::SetSockOpt(int ifd)
{
    if (ifd <= 0)
    {
        return false;
    }
    //
    int iOptFlag = 1;
    int iRet = setsockopt(ifd,  SOL_SOCKET, SO_REUSEADDR, (void*)& iOptFlag, sizeof(iOptFlag));
    if (iRet < 0)
    {
        SetErr(SOCK_ERR);
        MTS_LOG_ERROR("setsockopt SO_REUSEADDR fail");
        return false;
    }

    iRet = ::setsockopt(ifd, SOL_SOCKET, SO_KEEPALIVE,(void*)&iOptFlag,sizeof(iOptFlag));
    if (iRet < 0)
    {
        SetErr(SOCK_ERR);
        MTS_LOG_ERROR("setsockopt SO_KEEPALIVE fail");
        return false;
    }

    iRet = ::setsockopt(ifd, IPPROTO_TCP, TCP_NODELAY, (void*)&iOptFlag,sizeof(iOptFlag));
    if (iRet < 0)
    {
        SetErr(SOCK_ERR);
        MTS_LOG_ERROR("setsockopt TCP_NODELAY fail");
        return false;
    }
    return true;
}

int Sock::GerErr()
{
    return m_err;
}
//
void Sock::SetErr(int err)
{
    m_err = err;
}
//
int Sock::GetSockFd()
{
    return m_iFd;
}
//
int Sock::Send(const char* pSendBuf, int iLen)
{
    if (pSendBuf == NULL  || iLen < 0 || m_iFd <= 0)
    {
        return -1;
    }
    int iRet = ::send(m_iFd, pSendBuf, iLen, MSG_NOSIGNAL);
    if (iRet < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            SetErr(SOCK_AGAIN);
            return 0;
        }

        SetErr(SOCK_ERR);
        MTS_LOG_ERROR("send err");
        return -1;
    }
    return iRet;
}

int Sock::Recv(char* pRcvBuf, int iMaxLen)
{
    if (pRcvBuf == NULL || m_iFd <= 0)
    {
        return -1;
    }
    int iRet = ::recv(m_iFd, pRcvBuf, iMaxLen, 0);
    if (iRet < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            SetErr(SOCK_AGAIN);
            return -1;
        }
        SetErr(SOCK_ERR);
        MTS_LOG_ERROR("recv fail");
        return -2;
    }
    return iRet;
}

bool Sock::Listen()
{
    if (m_iFd <= 0)
    {
        return false;
    }

    SetSockOpt(m_iFd);

    struct sockaddr_in addrListen;
    memset(&addrListen, 0, sizeof(struct sockaddr_in));

    addrListen.sin_family = AF_INET;
    addrListen.sin_addr.s_addr = inet_addr(m_sSrvIp.c_str());
    addrListen.sin_port = ::htons(m_iSrvPort);
    unsigned int addrLen = sizeof(addrListen);

    int iRet = ::bind(m_iFd, (struct sockaddr *)&addrListen, addrLen);
    if (iRet < 0)
    {
        SetErr(SOCK_ERR);
        MTS_LOG_ERROR( "bind socket fail, port: %d, ip: %s, err msg: %s",
                      m_iSrvPort, m_sSrvIp.c_str(), strerror(errno) );
        return false;
    }

    iRet = ::listen(m_iFd, 1024);
    if (iRet < 0)
    {
        SetErr(SOCK_ERR);
        MTS_LOG_ERROR( "listen socket fail, port: %d, ip: %s",
                      m_iSrvPort, m_sSrvIp.c_str() );
        return false;
    }

    MTS_LOG_DEBUG( "bind ok, port: %d, ip: %s", m_iSrvPort, m_sSrvIp.c_str() );
    return true;
}
//
bool Sock::Connect()
{
    if (m_iFd <= 0)
    {
        return false;
    }

    struct sockaddr_in addrRemote;
    memset(&addrRemote, 0, sizeof(addrRemote));
    addrRemote.sin_family   = AF_INET; 
    addrRemote.sin_port     = ::htons(m_iSrvPort);
    inet_pton(AF_INET, m_sSrvIp.c_str(), &addrRemote.sin_addr);

    int iRet = ::connect(m_iFd, (struct sockaddr*)&addrRemote, sizeof(addrRemote));
    if (iRet < 0)
    {
        if (errno == EINPROGRESS)
        {
            MTS_LOG_INFO("connect is connecting...");
            return false;
        }
        MTS_LOG_ERROR("connect err");
        SetErr(SOCK_ERR);
        return false;
    }
    return true;
}

int Sock::SockAccept(struct sockaddr* pClientAddr)
{
    if (pClientAddr == NULL)
    {
        return -1;
    }

    socklen_t addrLen = sizeof(*pClientAddr);
    int iFd = ::accept(m_iFd, pClientAddr, &addrLen); 
    if (iFd < 0)
    {
        SetErr(SOCK_ERR);
        MTS_LOG_ERROR("accept fail");
        return iFd;
    }

    //add socket option
    if (SetSockOpt(iFd) == false)
    {
        MTS_LOG_ERROR("set opt fail on fd: %d", iFd);
        return -1;
    }
    if ( SetSockBlock(iFd, false) == false )
    {
        MTS_LOG_ERROR("set nonbock fail on fd: %d", iFd);
        return -1;
    }
    return iFd;
}

bool Sock::SetSockBlock(int ifd, bool isBlock)
{
    if (ifd <= 0)
    {
        return false;
    }

    int iOptFlags;
    iOptFlags = ::fcntl(ifd,F_GETFL,0);
    if (iOptFlags < 0)
    {
        return false;
    }
    if (isBlock == false)
    {
        if (::fcntl(m_iFd, F_SETFL, iOptFlags | O_NONBLOCK) < 0)
        {
            return false;
        }
    }
    return true;
}
//
