#include    "t_eventlisten.hpp"
#include    "mts_server.h"

ListenConn::ListenConn(int ifd, void* pData):ConnBase(ifd), m_pData((MtsTcp*)pData)
{
}

ListenConn::~ListenConn()
{
}

bool ListenConn::AddEvent(struct event_base *pEvBase, int iEvent, void *arg)
{
    if (pEvBase == NULL)
    {
        return false;
    }

    if (EV_READ & iEvent)
    {
        if ( 0  != event_assign(&m_stREvent, pEvBase, m_iFd, iEvent, ReadCallBack, this))
        {
            MTS_LOG_ERROR("event_assign() fail for read in listen event");
            return false;
        }
        if ( 0 !=  event_add(&m_stREvent, NULL) )
        {
            MTS_LOG_ERROR("event_add() fail for read in listen event");
            return false;
        }

        MTS_LOG_DEBUG("add event succ for read in listen event");
        return true;
    }
    else if (iEvent & EV_WRITE)
    {
        event_assign(&m_stWEvent, pEvBase, m_iFd, iEvent, WriteCallBack, this);
        event_add(&m_stWEvent, NULL);
        return true;
    }
    else 
    {
    }

    return false;
}

void ListenConn::WriteCallBack(int iFd, short sEvent, void *pData)
{
    //listen conn need not to write event.
}

void ListenConn::ReadCallBack(int iFd, short sEvent, void *pData) 
{
    ListenConn* pListenConn = (ListenConn*) pData; 
    if (pListenConn == NULL)
    {
        return;
    }

    MTS_LOG_DEBUG("listen recv read event, is client new conn");
    pListenConn->DoRead();
}

bool ListenConn::DoRead()
{
    MtsTcp* tcpSrv = (MtsTcp*) m_pData;
    return tcpSrv->Accept();
}

bool ListenConn::DelEvent(struct event_base *pEvBase, int iEvent, void *arg)
{
    MTS_LOG_DEBUG("call default op in DelEvent");
    return true;
}

