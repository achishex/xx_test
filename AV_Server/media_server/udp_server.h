/**
 * @file: udp_server.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x00001
 * @date: 2018-08-08
 */

#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_

#include  <event2/event.h>
#include  <event2/buffer.h>
#include  <event.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "avs_mts_log.h"
#include <string>

class MtsUdp
{
    public:
     MtsUdp(const std::string& sIp, unsigned short usPort);
     virtual ~MtsUdp();
    
     void UdpThreadCallback();

    private:
     bool Init();
     bool Run();
     static void EventCallback(int fd, short event, void *arg);

    private:
     std::string    m_sIp;
     unsigned short m_usPort;
     int            m_iSock;
     struct event*          m_pEvent;
     struct event_base*     m_pEventBase;
};

#endif
