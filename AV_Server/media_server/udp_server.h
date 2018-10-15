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

#include "LockQueue.h"
#include "t_PortPools.h"
#include <atomic>

using namespace util;
class MtsUdp
{
    public:
     MtsUdp(std::shared_ptr<LockQueue<std::shared_ptr<RelayPortRecord>>> queueSession);
     virtual ~MtsUdp();
    
     void UdpThreadCallback();
     void ShutDown()
     {
         m_bExit = true;
     }

    private:
     std::shared_ptr<LockQueue<std::shared_ptr<RelayPortRecord>>> m_pSessionQue;
     std::atomic<bool> m_bExit;
};

#endif
