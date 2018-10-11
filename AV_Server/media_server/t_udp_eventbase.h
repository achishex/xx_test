#ifndef _UdpServerEventBase_H_
#define _UdpServerEventBase_H_

#include <event2/event.h>
#include <event2/buffer.h>
#include <event.h>

#include <mutex>
#include <condition_variable>

#include "workPthread.h"
#include "t_udp_session.h"

class UdpServerEventBase : public WorkPthread
{
public:
    UdpServerEventBase( std::mutex* pLock, 
                       std::condition_variable* pCond, 
                       int* pReadyThreadNums, const int id = 0 );
   ~UdpServerEventBase();

    struct event_base * get_base() { return m_base ; }

protected:
    virtual int run() override;
private:
    std::mutex*                 m_pMutex;
    std::condition_variable*    m_pCond;
    int*                        m_pReadThreadNums;
    int                         m_id;
    struct event_base*          m_base ;
};

#endif //end define
