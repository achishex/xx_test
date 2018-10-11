#include "t_udp_eventbase.h"
#include <event2/thread.h>
#include "avs_mts_log.h"
#include <cassert>

UdpServerEventBase::UdpServerEventBase(std::mutex* pLock, 
                                       std::condition_variable* pCond, 
                                       int* pReadyThreadNums,  const int id )
  : m_id( id )
  , m_base( NULL )
{

    m_pMutex = pLock;
    m_pCond = pCond;
    m_pReadThreadNums = pReadyThreadNums;

    m_base  =  event_base_new();
    MTS_LOG_DEBUG( "[%d] worker event_base_new %p",  id,  m_base ) ;
    assert( m_base  !=  NULL );

    evthread_make_base_notifiable( m_base ) ;
}

UdpServerEventBase::~UdpServerEventBase()
{
    event_base_loopbreak( m_base );
    MTS_LOG_DEBUG("UdpServerEventBase stop loop");
    event_base_free( m_base );
}

void timeout_cb(int fd, short event, void *arg)

{
    struct timeval tv{86400, 0};
    event_add((struct event*)arg, &tv);
}


int UdpServerEventBase::run()
{
    struct event timeout;
    event_assign(&timeout, m_base, -1, 0, timeout_cb, (void*)&timeout);

    struct timeval tv{86400, 0};
    event_add(&timeout, &tv);
   
    {
        std::unique_lock<std::mutex> lock(*m_pMutex);
        ++(*m_pReadThreadNums);
        m_pCond->notify_one();
    }

    MTS_LOG_DEBUG("workPthread id: %02d start loop", m_id);

    event_base_dispatch( m_base );
    return 0;
}
