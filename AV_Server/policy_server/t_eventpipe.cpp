#include "t_eventpipe.hpp"
#include "t_worktask.hpp"

namespace T_TCP
{
    PipeConn::PipeConn(int ifd, void* pData): ConnBase(ifd), m_pData(pData)
    {
    }
    //
    PipeConn::~PipeConn()
    {
    }

    bool PipeConn::AddEvent(struct event_base *pEvBase, int iEvent, void *arg)
    {
        if (pEvBase == NULL)
        {
            return false;
        }
        if (EV_READ & iEvent)
        {
            event_assign(&m_stREvent, pEvBase, m_iFd, iEvent, ReadCallBack, this);
            event_add(&m_stREvent, NULL); 
        }
        else if (iEvent & EV_WRITE)
        {
            event_assign(&m_stWEvent, pEvBase, m_iFd, iEvent, WriteCallBack, this);
            event_add(&m_stWEvent, NULL);
        }
        else 
        {

        }
        return true;
    }

    void PipeConn::WriteCallBack(int iFd, short sEvent, void *pData)
    {
    }

    void PipeConn::ReadCallBack(int iFd, short sEvent, void *pData)
    {
        char buf[4] = {0};
        int iRet = ::read(iFd, buf, sizeof(buf));
        if ( iRet == -1)
        {
            PL_LOG_ERROR("read buf from pipe fail, err msg: %s", strerror(errno));
            return ;
        }

        PipeConn* pPipeConn = (PipeConn*) pData; 
        if (pPipeConn == NULL)
        {
            PL_LOG_ERROR("pipe read event call back pdata is numm");
            return;
        }

        pPipeConn->DoRead(buf,iRet);
    }

    void PipeConn::DoRead(char *pData, int iLen)
    {
        WorkerTask* pTask = (WorkerTask*)m_pData;
        pTask->NotifyRecvConn(pData, iLen);
    }
    
    //FixMe:
    bool PipeConn::DelEvent(struct event_base *pEvBase, int iEvent, void *arg)
    {
        return true;
    }
}
