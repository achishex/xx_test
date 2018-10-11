#include "policy_log.h"
#include "t_eventbase.hpp"

#include <unistd.h>
#include <iostream>
namespace T_TCP
{
    ConnBase::ConnBase(int iFd):m_iFd(iFd) 
    {
    }

    //
    ConnBase::~ConnBase()
    {
        event_del(&m_stREvent);
        event_del(&m_stWEvent);
        PL_LOG_DEBUG("close conn fd: %d", m_iFd);
        ::close(m_iFd);
        m_iFd = -1;
    }
    //
}
