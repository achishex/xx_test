#include "t_pthread_manager.h"
#include <event2/thread.h>
#include "avs_mts_log.h"
#include "t_mts_udp_proto.h"
#include "configXml.h"

PthreadManager::PthreadManager()
    : m_cur_thread_index( 0 ), m_iPoolSize(0),
    m_iReadyThreadNums(0)
{
    evthread_use_pthreads();
}

PthreadManager::~PthreadManager()
{
}

int PthreadManager::Init( int workThreadCount )
{
    if( workThreadCount  <  1 ) { workThreadCount  =  1 ; }
    m_iPoolSize = workThreadCount;
    
    for ( int i = 0;  i < workThreadCount;  ++i )
    {
        UdpServerEventBase* workThread  = new UdpServerEventBase(&m_mutex_start, 
                                                         &m_cond_start,
                                                         &m_iReadyThreadNums,
                                                         i + 1 ); // thread
        if ( NULL == workThread)
        {
            break ;
        }

        workThread->start();
        m_thread_list.push_back( workThread );
    }

    WaitWorkThreadRegiste();
    MTS_LOG_DEBUG("all threads start done, thread nums: %d", workThreadCount);
    return 0 ;
}

void PthreadManager::WaitWorkThreadRegiste()
{
    std::unique_lock<std::mutex> lock(m_mutex_start);
    while( m_iReadyThreadNums < m_iPoolSize ) 
    {
        m_cond_start.wait( lock );
    }
}

void PthreadManager::DispatchUdpMsg( void * buf,  int buf_len )
{
    if( ( buf  ==  NULL )  ||  ( buf_len  !=  sizeof( RelayPortRecord ) ) )
    {
        MTS_LOG_DEBUG( "bad size: buf = %p,len = %d, expected = %d",  
                      buf, buf_len, sizeof( RelayPortRecord) );
        return ;
    }

    RelayPortRecord* pRelayItem  =  static_cast< RelayPortRecord* >( buf ) ;

    UdpSession          *pNowSession    =  NULL ;
    UdpServerEventBase  *pEventBase     =  NULL ;
    
    unsigned short usCmd    = pRelayItem->usRelayCmd;
    std::string sSessionId( pRelayItem->m_sSessId );

    if (sSessionId.empty())
    {
        MTS_LOG_ERROR("session id inparam is empty");
        return ;
    }
    MTS_LOG_DEBUG("udp thread call, disptach udp msg, sessionid: %s", sSessionId.c_str());

    auto sessionIt = m_mpRelaySession.find( sSessionId );
    if (sessionIt != m_mpRelaySession.end() )
    {
        pNowSession = sessionIt->second;
    }
    switch( usCmd )
    {
        case UDP_CMD_ALLOC_PORT:
        {
            if (pNowSession == NULL)
            {
                pEventBase  =   get_idle_thread();
                pNowSession =   EventManager::Instance()->acquire();
                bool iRet   =   pNowSession->Init(*pRelayItem, pEventBase->get_base(),
                                                 ConfigXml::Instance()->getValue("MediaServer","IP"),
                                                 ::atoi( ConfigXml::Instance()->getValue("MediaServer", "Port" ).c_str()));
                if (iRet == false)
                {
                    MTS_LOG_ERROR("new session fail on session id: %s", sSessionId.c_str());
                    EventManager::Instance()->release(pNowSession);
                    break;
                }
                m_mpRelaySession[sSessionId] = pNowSession;
            }
            break;
        }
        case UDP_CMD_RELEASE_PORT:
        {
            if (pNowSession)
            {
                pNowSession->NotifyCloseSession();
                m_mpRelaySession.erase(sessionIt);
                //not del session
            }
            else
            {
                MTS_LOG_ERROR("session is null, session id: %s", sSessionId.c_str());
            }
            break;
        }
        default:
            break;
    }
}

UdpServerEventBase* PthreadManager::get_idle_thread() // round robin
{
    UdpServerEventBase  *  base =  m_thread_list[ m_cur_thread_index ] ;
    ++ m_cur_thread_index ;
    if( m_cur_thread_index  >=  int( m_thread_list.size() ) ) { m_cur_thread_index = 0 ; }
    return base ;
}
