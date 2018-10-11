#ifndef _UDP_CLIENT_H_
#define _UDP_CLIENT_H_

#include <iostream>
#include <string>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
 
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 
#include <errno.h>
#include <utility>
#include <fcntl.h>
#include <unistd.h>

#include "avs_mts_log.h"

class UdpClient
{
public:
    UdpClient();
    UdpClient(int localport);
    //just used for bind udp 
    UdpClient(string ip, int localport);
    UdpClient(const char *ip, int localport, bool do_connect = true);
    ~UdpClient();
    int sendTo(string dstIp, int dstPort, string str);
    int sendTo(string dstIp, int dstPort, const void* buf, int len);
	int SendData(const char *buf, int len);
	void Init(const char *ip, int localport, bool do_connect);
public:
    int		m_sock;
    string	m_ip;
    int		m_port;
    struct sockaddr_in m_remoteAddr;
    bool	m_doConnect;
private:
    UdpClient(const UdpClient &copy);
    UdpClient &operator=(const UdpClient &copy);
};


#endif
