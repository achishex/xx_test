#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rutil/Logger.hxx"
#include "ReportLoadThread.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO
using namespace resip;
using namespace repro;
using namespace std;

RL_WorkThread::RL_WorkThread(std::shared_ptr<RL_Work> pRlWork): m_pRlWork(pRlWork)
{
}

RL_WorkThread::~RL_WorkThread()
{
    shutdown();
    join();
}

void RL_WorkThread::thread()
{
    if (m_pRlWork && !isShutdown())
    {
        InfoLog( << "report load thread begin to run ..." );
        m_pRlWork->onStart();
        while( m_pRlWork && !isShutdown() )
        {
            m_pRlWork->Process();
        }
        InfoLog( << "report load thread exit now ...." );
    }
}
