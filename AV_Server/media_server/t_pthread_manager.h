#ifndef PthreadManager_H
#define PthreadManager_H

#include <vector>
#include <cstring>
#include <mutex>
#include <condition_variable>

#include "LibSingleton.h"
#include "t_udp_eventbase.h"


class PthreadManager: public CSingleton <PthreadManager> // 管理线程类
{
 public:
    ~PthreadManager();
    PthreadManager();
     
    int  Init( int workThreadCount = 10 );              /**<  called in main thread*/
    void DispatchUdpMsg( void * buf,  int buf_len );    /**< called in udp thread */

 private:
    UdpServerEventBase* get_idle_thread();
    void WaitWorkThreadRegiste();
    std::mutex m_mutex_start;
    std::condition_variable m_cond_start;
 private:
    int                                     m_cur_thread_index;
    int                                     m_iPoolSize;
    int                                     m_iReadyThreadNums;
    std::vector< UdpServerEventBase * >     m_thread_list;
    std::map<std::string, UdpSession*>      m_mpRelaySession;
};

#endif
