#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rutil/Logger.hxx"
#include "ReportLoadContent.hxx"

using namespace repro;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

ReportLoadContent::ReportLoadContent(): m_iCurTm(time(NULL))
{
}

ReportLoadContent::~ReportLoadContent()
{
}

void ReportLoadContent::UpdateLoadHost(const std::string& sHost)
{
    resip::WriteLock w(mMutex);
    m_loadCont.m_sLocalHost = sHost;
}

void ReportLoadContent::UpdateLoadPort(int iPort)
{
    resip::WriteLock w(mMutex);
    m_loadCont.m_iLocalPort = iPort;
}

void ReportLoadContent::IncrConnNums()
{
    int curTm = time(NULL);
    int num = 0;
    {
        resip::WriteLock w(mMutex);
        if ((curTm - m_iCurTm)> 0)
        {
            m_loadCont.m_iConnNums = 0;
            m_iCurTm = curTm;
        }
        m_loadCont.m_iConnNums++;
        m_loadCont.m_iTotalConn++;
        num = m_loadCont.m_iConnNums;
    }
    DebugLog( <<"now received conn nums: " << num );
}


ReportLoadContent::LoadContent ReportLoadContent::GetLoad()
{
    resip::WriteLock w(mMutex);
    ReportLoadContent::LoadContent loadCont;
    loadCont.m_sLocalHost   = m_loadCont.m_sLocalHost;
    loadCont.m_iLocalPort    = m_loadCont.m_iLocalPort;
    loadCont.m_iConnNums    = m_loadCont.m_iConnNums;
    loadCont.m_iTotalConn   = m_loadCont.m_iTotalConn;
    return loadCont;
}
