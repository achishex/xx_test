#include "t_udp_client.h"

UdpClient::UdpClient():m_sock(-1), m_ip("0.0.0.0"), m_port(0), m_doConnect(false)
{
    bzero(&m_remoteAddr, sizeof(m_remoteAddr));
    m_sock = socket(AF_INET,SOCK_DGRAM,0);
    if ( m_sock < 0 )
    {
        MTS_LOG_ERROR("create socket failed, errmsg: %s", strerror(errno));
    }
}

UdpClient::UdpClient(int localPort): m_sock(-1),m_ip("0.0.0.0"), m_port(localPort),
	m_doConnect(true)
{
    bzero(&m_remoteAddr, sizeof(m_remoteAddr));
    m_sock = socket(AF_INET,SOCK_DGRAM,0);
    if ( m_sock < 0 )
    {
        MTS_LOG_ERROR("create socket failed, errmsg: %s", strerror(errno));
        return;
    }

    struct sockaddr_in localAddr;
    localAddr.sin_family        = AF_INET;
    localAddr.sin_port          = htons(m_port);
    localAddr.sin_addr.s_addr   = INADDR_ANY;
    bzero(&localAddr.sin_zero, sizeof(localAddr.sin_zero));
    
	int _ret = bind(m_sock, (sockaddr*)&localAddr, sizeof(localAddr));
    if ( 0 != _ret )
	{
        close(m_sock);
        m_sock = -1;
		MTS_LOG_ERROR("bind udp sockaddr fail, errmsg: %s", strerror(errno));
		return;
	}
}

UdpClient::UdpClient(string ip, int localPort)
  : m_sock(-1), m_ip( ip ), m_port(localPort),m_doConnect(false)
{
    bzero(&m_remoteAddr, sizeof(m_remoteAddr));
    m_sock = socket(AF_INET,SOCK_DGRAM,0);
    if ( m_sock < 0 )
    {
		MTS_LOG_ERROR("create sock fail, err msg: %s", strerror(errno));
        return;
    }

    struct sockaddr_in localAddr;
    if ( m_ip.empty() )
    {
        localAddr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        localAddr.sin_addr.s_addr = inet_addr(const_cast<char*>(m_ip.c_str()));
    }

    localAddr.sin_family    = AF_INET;
    localAddr.sin_port      = htons(m_port);
    bzero(&localAddr.sin_zero, sizeof(localAddr.sin_zero));
    
	int _ret = bind(m_sock, (sockaddr*)&localAddr, sizeof(localAddr));
    if ( 0 != _ret )
    {
        close( m_sock );
        m_sock = -1;
        MTS_LOG_ERROR("bind udp sockaddr failed!, errmsg: %s", strerror(errno));
        return;
    }
}

UdpClient::UdpClient(const char *ip, int localport, bool do_connect)
{
	Init(ip, localport, do_connect);
}

void UdpClient::Init(const char *ip, int localport, bool do_connect)
{
	m_ip.clear();
	m_ip.append(ip);
	m_port = localport;
    
	m_sock = socket(AF_INET,SOCK_DGRAM,0);
    if ( m_sock < 0 )
    {
		MTS_LOG_ERROR("create socket failed, errmsg: %s", strerror(errno));
        return;
    }
	static const int SND_BUF_SIZE = 2*1024*1024;

    if ( setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, & SND_BUF_SIZE, sizeof(SND_BUF_SIZE)) < 0)
    {
        MTS_LOG_ERROR( "setsockopt send_buf_size=%d failed: %d",  SND_BUF_SIZE,  errno );
    }

	m_remoteAddr.sin_family = AF_INET;
    m_remoteAddr.sin_addr.s_addr = inet_addr(ip);
    m_remoteAddr.sin_port = htons(localport);
	m_doConnect = do_connect;
    if(m_doConnect == true)
    {
    	if(::connect(m_sock, (struct sockaddr *)&m_remoteAddr, sizeof(m_remoteAddr)) < 0)
   	 	{
   	 		MTS_LOG_ERROR("udp connect failed, err: %s", strerror(errno));
        	m_doConnect = false;
      	}
    }
}

int UdpClient::SendData(const char *buf, int len)
{
	if (m_sock <= 0)
	{
        MTS_LOG_ERROR("created socket in invalid");
        return -1;
	}
	if(m_doConnect == true)
	{
		return sendto(m_sock, buf, len, 0, NULL, 0);
	}
	else
	{
		return sendto(m_sock, buf, len, 0, (struct sockaddr *)&m_remoteAddr, sizeof(struct sockaddr_in));
	}
}

int UdpClient::sendTo(string dstIp, int dstPort, string str)
{
    return sendTo(dstIp, dstPort, str.c_str(), str.length());
}

int UdpClient::sendTo(string dstIp, int dstPort, const void* buf, int len)
{
    struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family      = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(const_cast<char*>(dstIp.c_str()));
    remote_addr.sin_port        = htons(dstPort);

    if ( m_sock < 0 )
    {
        MTS_LOG_ERROR("the server sock was uninitialized");
        return -1;
    }

    int senLen = sendto(m_sock, buf, len, 0, (sockaddr*)&remote_addr, sizeof(remote_addr));
	if ( senLen < 0)
    {
		MTS_LOG_ERROR("the udp send buf error!");
        return -1;
	}

	return senLen;
}

UdpClient::~UdpClient()
{
    if (m_sock > 0)
    {
        ::close(m_sock);
    }
}
