#include "t_timer_report.h"
#include <stdio.h>
#include <stdlib.h>

TimerReport::TimerReport( struct event_base * base,  int iseconds,  CallBack func,  void * arg )
    :m_event(NULL),m_timeout(NULL)
{
    m_timeout = new timeval;
    evutil_timerclear( m_timeout );
    m_timeout->tv_sec = iseconds;

    m_event  =  event_new( base, -1, EV_PERSIST, func, arg );
    if( m_event )
    {
        evtimer_add( m_event,  m_timeout );
    }
}

TimerReport::~TimerReport()
{
    evtimer_del( m_event ); 
    delete m_timeout ;
    event_free( m_event  );
}
