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
                            std::atomic<int>* pInitNums, 
                            int iWorkId): PthreadBase(pInitLock, pInitCond, pInitNums), 
                            m_iNotifyRecvFd(-1),
                            m_iNofitySendFd(-1), 
                            m_PthreadEventBase(NULL),
                            m_pConnItemList(NULL),
                            m_pConnItemListFree(NULL),
                            m_iWorkId(iWorkId), m_pThreadPool(NULL)
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

        if (m_PthreadEventBase)
        {
            event_base_free(m_PthreadEventBase);
        }
        m_PthreadEventBase = NULL;
    }

    int WorkerTask::Init() 
    {
        if ( m_reqConnQueue != nullptr )
        {
            PL_LOG_INFO("worker task is for dispatch req conn, need not init event, pid: %lu", GetThreadId());
            return 0;
        }
        PL_LOG_INFO("is worker task, pid: %lu, go on....", GetThreadId());

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
        
        MainLoop();

        PL_LOG_INFO("thread: %lu exit", GetThreadId());
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
            PL_LOG_DEBUG("from pipe, recv new conn fd: %d, this work thread id: %d", iConnFd, GetWorkId());

            m_mpAcceptConn.insert(std::pair<int, ConnBase*>(iConnFd, pAcceptConn)); //when close iFd, delete pConn
            return true;
        }


        switch(pBuf[0])
        {
            case 'c':
                {
                    PL_LOG_ERROR("not been used, dangous!!!!");
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


    void WorkerTask::SetData( std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType>>>& QConnNode )
    {
        m_reqConnQueue = QConnNode;
    }

    void WorkerTask::MainLoop()
    {
        if (m_reqConnQueue == nullptr)
        {
            PL_LOG_INFO("into task work main loop,pid: %lu", GetThreadId());
            event_base_dispatch(m_PthreadEventBase);
            return ;
        }

        PL_LOG_INFO("into dispatch req conn main loop, pid: %lu", GetThreadId());

        int iTmoutConnVal = 30; //60 second.
        std::shared_ptr<ConnNodeType> ptrconnNode(nullptr);
        while(true)
        {
            ptrconnNode = (nullptr);
            m_reqConnQueue->Get( &ptrconnNode );
            if (ptrconnNode == nullptr)
            {
                PL_LOG_ERROR("get new conn is empty node");
                continue;
            }
            PL_LOG_DEBUG("get new conn from queue, conn: %s", ptrconnNode->ToString().c_str());
            
            int iDiffTm = time(NULL) - ptrconnNode->m_tmAccept;
            if ( iDiffTm > iTmoutConnVal)
            {
                PL_LOG_ERROR("conn is old, close fd, diff tm: %ds, conn: %s", iDiffTm, ptrconnNode->ToString().c_str());
                ::close(ptrconnNode->m_iAccptFd);
                continue;
            }

            //从业务/工作线程池中分配一个事件线程，用于独立接收报文和发送报文.
            //so,需要有个线程池.
            WorkerTask* pFreeTaskWork = m_pThreadPool->AllocateThread();
            if (pFreeTaskWork == NULL)
            {
                ::close(ptrconnNode->m_iAccptFd);
                PL_LOG_ERROR("get free work task thread from pool fail, conn client ip: %s", 
                             ptrconnNode->m_sPeerIp.c_str());
                continue;
            }
            
            char buf[4] = {0};
            unsigned int iNetFd = ::htonl(ptrconnNode->m_iAccptFd);
            memcpy(buf, &iNetFd, sizeof(buf));
            if (false == pFreeTaskWork->NotifySendConn(buf, sizeof(buf)))
            {
                PL_LOG_ERROR("send req conn fd fail, fd: %d", ptrconnNode->m_iAccptFd);
                close(ptrconnNode->m_iAccptFd);
                continue;
            }

            PL_LOG_DEBUG("dispatch fd succ by pipe, dst worker thread id: %d, [ accept fd, remote client ip ] => [ %d,%s ]", 
                        pFreeTaskWork->GetWorkId(), ptrconnNode->m_iAccptFd, ptrconnNode->m_sPeerIp.c_str());
        }
    }

    void WorkerTask::SetThreadPool( PthreadPools<WorkerTask, std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType>>>>* pThreadPool )
    {
        m_pThreadPool = pThreadPool;
    }
}
