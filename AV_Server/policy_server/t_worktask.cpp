#include "t_worktask.hpp"
#include  <unistd.h>
#include "t_eventpipe.hpp"
#include "t_eventconn.hpp"
#include "t_node_load.h"

#include "policy_log.h"

namespace T_TCP
{
    WorkerTask::WorkerTask():m_iNotifyRecvFd(-1),
                            m_iNofitySendFd(-1),
                            m_PthreadEventBase(NULL)
    { }

    WorkerTask:: WorkerTask(pthread_mutex_t* pInitLock,
                            pthread_cond_t* pInitCond, 
                            int* pInitNums, 
                            int iWorkId): PthreadBase(pInitLock, pInitCond, pInitNums), 
                            m_iNotifyRecvFd(-1),
                            m_iNofitySendFd(-1), 
                            m_PthreadEventBase(NULL),
                            m_pConnItemList(NULL),
                            m_pConnItemListFree(NULL),
                            m_iWorkId(iWorkId)
    { }

    WorkerTask::~WorkerTask()
    {
        if (m_iNotifyRecvFd > 0)
        {
            ::close(m_iNotifyRecvFd); m_iNotifyRecvFd = -1;
        }

        if (m_iNofitySendFd > 0)
        {
            ::close(m_iNofitySendFd);  m_iNofitySendFd = -1;
        }


        for (auto it = m_mpAcceptConn.begin(); it != m_mpAcceptConn.end(); )
        {
            if (it->second)
            {
                delete it->second;
            }
            if (it->first > 0)
            {
                ::close(it->first);
            }
            m_mpAcceptConn.erase(it++);
        }

        if (m_pPipNotifyConn)
        {
            delete m_pPipNotifyConn; 
            m_pPipNotifyConn = NULL;
        }

        event_base_free(m_PthreadEventBase);
        m_PthreadEventBase = NULL;
    }

    int WorkerTask::Init() 
    {
        struct event_config *ev_config = event_config_new();
        event_config_set_flag(ev_config, EVENT_BASE_FLAG_NOLOCK);
        m_PthreadEventBase = event_base_new_with_config(ev_config);
        event_config_free(ev_config);

        PL_LOG_DEBUG("event base init, event base addr: %p", m_PthreadEventBase);
        if (m_PthreadEventBase == NULL)
        {
            m_bRun = false;
            return -1;
        }

        int fds[2];
        if (::pipe(fds))
        {
            PL_LOG_ERROR("create notify pip err");
            return -1;
        }
        //
        m_iNotifyRecvFd =  fds[0];
        m_iNofitySendFd =  fds[1];

        m_pPipNotifyConn = new PipeConn(m_iNotifyRecvFd, this);
        AddRConn(m_pPipNotifyConn);
        
        PL_LOG_DEBUG("add read event into pipe, notify pipe send fd: %d,"
                     " notify pipe recv fd: %d",
                     m_iNofitySendFd, m_iNotifyRecvFd);
        return 0;
    }

    void WorkerTask::AddRConn(ConnBase* pConn)
    {
        pConn->AddEvent(m_PthreadEventBase, EV_READ | EV_PERSIST, this);
    }

    int WorkerTask::main()
    {
        RegistePthreadToPool();
        
        event_base_dispatch(m_PthreadEventBase);
        PL_LOG_INFO("thread: %d  exit", GetThreadId());
        m_bRun = false;
        return 0;
    }

    bool WorkerTask::NotifyRecvConn(char *pBuf, int iLen)
    {
        if (pBuf == NULL)
        {
            return false;
        }

        if (iLen < 1)
        {
            PL_LOG_ERROR("pipe recv item len less than 1");
            return false;
        }

        if (iLen == 4)
        {
            int iConnFd = ::ntohl(*(unsigned int*)(pBuf));
            
            if (iConnFd <= 0)
            {
                PL_LOG_ERROR("pipe recv conn new fd less than 0");
                return false;
            }

            AcceptConn* pAcceptConn = new AcceptConn(iConnFd, this);
            AddRConn(pAcceptConn);
            PL_LOG_DEBUG("from pipe, recv new conn fd: %d", iConnFd);

            m_mpAcceptConn.insert(std::pair<int, ConnBase*>(iConnFd, pAcceptConn)); //when close iFd, delete pConn
            return true;
        }


        switch(pBuf[0])
        {
            case 'c':
                {
#if 0
                    if (m_pConnItemList == NULL || m_pConnItemListFree == NULL)
                    {
                        return false;
                    }
                    //从队列中取出一个新连接请求.
                    QueueItem item;
                    bool bRet = m_pConnItemList->DeQue(&item);
                    if (bRet == false)
                    {
                        PL_LOG_ERROR("get conn node from pip fail");
                        return false;
                    }

                    int iFd = 0;
                    iFd = item.iConnFd;
                    //发起一个连接, 需要从连接池中分配一个空闲连接
                    AcceptConn* pAcceptConn = new AcceptConn(iFd, this);
                    AddRConn(pAcceptConn);
                    PL_LOG_DEBUG("from pipe, new conn fd: %d", iFd);

                    m_mpAcceptConn.insert(std::pair<int, ConnBase*>(iFd, pAcceptConn)); //when close iFd, delete pConn

                    item.iConnFd = -1;
                    m_pConnItemListFree->EnQue(item);
#endif
                }
                break;

            case 't':
                {
                    PL_LOG_DEBUG("recv timer cmd to write nodeload to db");
                    FlushNodeLoadToDb(); 
                }
                break;

            default:
                break;
        }
        return true;
    }

    void WorkerTask::DeleteAcceptConn(int fd)
    {
        if (fd <=0)
        {
            return ;
        }
        //
        std::map<int, ConnBase*>::iterator it = m_mpAcceptConn.find(fd);
        if (it != m_mpAcceptConn.end())
        {
            delete it->second;
            m_mpAcceptConn.erase(it);
        }
    }

    bool WorkerTask::NotifySendConn(FreeLockQue<QueueItem>* pItemList, FreeLockQue<QueueItem>* pItemListFree)
    {
        if (pItemList == NULL)
        {
            return false;
        }
        char buf[1];
        buf [0] = 'c';
        if (::write(m_iNofitySendFd, buf, 1 ) != 1)
        {
            PL_LOG_ERROR("write cmd c on notifysendcmd pipe fail");
            return false;
        }
        m_pConnItemList = pItemList;
        m_pConnItemListFree = pItemListFree;
        return true;
    }

    bool WorkerTask::NotifySendConn(char cmdChar)
    {
        if (::write(m_iNofitySendFd, &cmdChar,1) != 1)
        {
            PL_LOG_ERROR("write cmd %c on notifysend pipe fail", cmdChar);
            return false;
        }
        return true;
    }

    bool WorkerTask::NotifySendConn(const char *pBuf, int iLen)
    {
        if (::write(m_iNofitySendFd, pBuf, iLen) != iLen)
        {
            PL_LOG_ERROR("write fd to notify pipe fail, err msg: %s", strerror(errno));
            return false;
        }
        return true;
    }

    void WorkerTask::FlushNodeLoadToDb()
    {
        NodeLoadCollection::Instance()->FlushDB();
    }
}
