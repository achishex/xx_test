#include "avs_mts_log.h"
#include "t_eventbase.hpp"

#include <unistd.h>
#include <iostream>

ConnBase::ConnBase(int iFd):m_iFd(iFd) 
{
}

//
ConnBase::~ConnBase()
{
    DelREvent();
    DelWEvent();
    MTS_LOG_DEBUG("close conn fd: %d", m_iFd);
    ::close(m_iFd);
    m_iFd = -1;
}

void ConnBase::DelWEvent()
{
    if ( event_get_events(&m_stWEvent) & EV_WRITE)
    {
        event_del(&m_stWEvent);
    }
}
void ConnBase::DelREvent()
{
    if (event_get_events(&m_stREvent) & EV_READ)  
    {
        event_del(&m_stREvent);
    }
}
//
