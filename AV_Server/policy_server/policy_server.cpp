#include "policy_server.hpp"
#include "t_eventlisten.hpp"
#include "StringTools.hpp"
#include "t_node_load.h"
namespace  T_TCP
{
    TcpSrv::TcpSrv(const std::string& sIp, int iPort, int threadNums): m_bInit(false), 
                   p_mListenSock(NULL), m_sLocalIp(sIp), m_iLocalPort(iPort), m_pEventBase(NULL),
                    m_pThreadPool(NULL),m_pDispathConnThreadPool(NULL), m_iThreadPoolNums(threadNums), 
                    p_mListenConn(NULL),m_iConnReqQueueLen(1024), 
                    m_pConnReqQueue(nullptr)
    {
        try {
            Init();
        } catch (...)
        {
            m_bInit = false;
            return ;
        }

        m_iAcceptCountNow = m_iAcceptCountLast = 0;
        pEv_Tmr = NULL;

        if (m_iThreadPoolNums == 0)
        {
            PL_LOG_INFO("work thread num is 0, set def num: 4");
            m_iThreadPoolNums  = 4;
        }
    }

    //
    TcpSrv::~TcpSrv()
    {
        m_bInit = false;
        if (p_mListenSock)
        {
            delete p_mListenSock;
            p_mListenSock = NULL;
        }

        if (m_pThreadPool)
        {
            delete m_pThreadPool;
            m_pThreadPool = NULL;
        }

        if (m_pDispathConnThreadPool)
        {
            delete m_pDispathConnThreadPool;
            m_pDispathConnThreadPool = NULL;
        }

        if (p_mListenConn)
        {
            delete p_mListenConn;
            p_mListenConn = NULL;
        }

        if (m_pAcceptConnListFree)
        {
            delete m_pAcceptConnListFree;
            m_pAcceptConnListFree = NULL;
        }

        if (m_pAcceptConnListWorking)
        {
            delete m_pAcceptConnListWorking;
            m_pAcceptConnListWorking = NULL;
        }
        if (pEv_Tmr)
        {
            delete pEv_Tmr;
            pEv_Tmr = NULL;
        }

        //
        if (m_pEventBase)
        {
            event_base_free(m_pEventBase); 
            m_pEventBase = NULL;
        }
    }

    //
    bool TcpSrv::Init()
    {
        if (p_mListenSock == NULL)
        {
            p_mListenSock = new Sock(m_sLocalIp, m_iLocalPort);
        }

        bool bRet = p_mListenSock->Listen();
        if (bRet == false)
        {
            PL_LOG_ERROR("Listen sock fail, err no: %d", p_mListenSock->GerErr());
            return false;
        }

        struct event_config *ev_config = event_config_new();
        event_config_set_flag(ev_config, EVENT_BASE_FLAG_NOLOCK);
        m_pEventBase = event_base_new_with_config(ev_config);
        event_config_free(ev_config);

        PL_LOG_DEBUG("tcp srv event base init succ, base event addr: %p, listen fd: %d",
                     m_pEventBase, p_mListenSock->GetSockFd());
       

        std::string sRedisIp = ConfigXml::Instance()->getValue("RedisNode", "IP");
        int iRedisPort = ::atoi(ConfigXml::Instance()->getValue("RedisNode","Port").c_str());
        int iMaxConnNum = ::atoi(ConfigXml::Instance()->getValue("RedisNode","MaxConnNum").c_str());
        int iDbIndex = ::atoi(ConfigXml::Instance()->getValue("RedisNode","DbIndex").c_str());
        int iConnIdleMaxMs = ::atoi(ConfigXml::Instance()->getValue(
                                    "RedisNode","ConnIdleMaxMs").c_str());

        PL_LOG_DEBUG("redis conf => ip: %s, port: %d, maxconnnum: %d, dbindex: %d, connidlemaxms: %d",
                     sRedisIp.c_str(), iRedisPort, iMaxConnNum, iDbIndex, iConnIdleMaxMs);

        m_pRedisHandle = std::make_shared<FS_RedisInterface>(iMaxConnNum, 
                                                             iConnIdleMaxMs,
                                                             sRedisIp,
                                                             iRedisPort,
                                                             iDbIndex);
        //
        NodeLoadCollection::Instance()->SetRedisHandle(m_pRedisHandle);
        if ( false == NodeLoadCollection::Instance()->LoadAllNodeLoad() )
        {
            PL_LOG_ERROR("load node load from db fail");
        }
        
        m_pAcceptConnListFree = new FreeLockQue<QueueItem>(ConnectFreeListLen);
        m_pAcceptConnListWorking = new FreeLockQue<QueueItem>(ConnectFreeListLen);

        m_iTmerNotifyTms = ::atoi(ConfigXml::Instance()->getValue(
                                "PolicyServer", "NotifyTimerTm").c_str());
        if (m_iTmerNotifyTms == 0)
        {
            m_iTmerNotifyTms = 5*60;
        }
        PL_LOG_DEBUG("notify timer tm: %ds", m_iTmerNotifyTms);
        
        int iConnReqQueueLen = ::atoi(ConfigXml::Instance()->getValue(
                "PolicyServer","ConnReqQueueLen").c_str());
        if (iConnReqQueueLen <=0)
        {
            PL_LOG_ERROR("get conf: PolicyServer-ConnReqQueueLen fail,use default len: %d", m_iConnReqQueueLen);
        }
        else
        {
            m_iConnReqQueueLen = iConnReqQueueLen;
        }

        PL_LOG_DEBUG("connect req queue len: %d, now create conn req queue", m_iConnReqQueueLen);
        m_pConnReqQueue = std::make_shared<LockQueue<std::shared_ptr<ConnNodeType>>>(m_iConnReqQueueLen);
        if (!m_pConnReqQueue)
        {
            PL_LOG_ERROR("create conn queue fail");
            return false;
        }
        
        PL_LOG_DEBUG("tcp srv init succ");
        m_bInit = true;
        return true;
    }

    bool TcpSrv::RegisteAcceptConn()
    {
       p_mListenConn = new ListenConn(p_mListenSock->GetSockFd(), (void*)this); 
       if (p_mListenConn == NULL)
       {
           return false;
       }
        
       bool bRet = p_mListenConn->AddEvent(m_pEventBase, EV_READ|EV_PERSIST, this);
       if (false == bRet)
       {
            delete p_mListenConn;
            p_mListenConn = NULL;
            PL_LOG_ERROR("add read event in listen conn event set fail");
            return false;
       }

       PL_LOG_DEBUG( "register read event into listen event set succ, listen fd: %d",
                    p_mListenSock->GetSockFd() );
       return true;
    }

    bool TcpSrv::Run()
    {
        if (m_bInit == false)
        {
            PL_LOG_ERROR("tcp srv init fail in Run()");
            return false;
        }

        //m_iThreadPoolNums is work thread nums, 1 is manager cmd thread nums
        m_pThreadPool = new PthreadPools<WorkerTask, std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType>>>>(m_iThreadPoolNums + 1);
        if ( !m_pThreadPool->CreateThreads() )
        {
            PL_LOG_ERROR("create worker threads fail");
            return false;
        }
        std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType>>> nullNode(nullptr);
        m_pThreadPool->SetThreadContent(nullNode);
        if ( !m_pThreadPool->StartAllThreads() )
        {
            PL_LOG_ERROR( "start task thread fail, thread nums: %d, timer thread nums: %d",
                         m_iThreadPoolNums, 1 );
            return false;
        }
        PL_LOG_INFO("worker threads nums: %d, timer thread nums: %d", m_iThreadPoolNums, 1);


        m_pDispathConnThreadPool = new PthreadPools<WorkerTask, std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType>>>>(m_iThreadPoolNums);
        if ( !m_pDispathConnThreadPool->CreateThreads() )
        {
            PL_LOG_ERROR("create dispatch conn thread pool fail");
            return false;
        }
        m_pDispathConnThreadPool->SetThreadContent(m_pConnReqQueue);
        m_pDispathConnThreadPool->SetTheadPoolEachThead(m_pThreadPool);

        if ( !m_pDispathConnThreadPool->StartAllThreads() )
        {
            PL_LOG_ERROR("start dispatch conn req thead fail, thread nums: %d", m_iThreadPoolNums);
            return false;
        }

        PL_LOG_INFO("dispatch conn req thread nums: %d", m_iThreadPoolNums);

        if (RegisteAcceptConn() == false)
        {
            PL_LOG_ERROR("registe accept conn fail");
            return false;
        }

        if (RegisteNotifyTimer() == false)
        {
            PL_LOG_ERROR("register notify timer fail");
            return false;
        }

        //
        int iRet = event_base_loop(m_pEventBase, 0);
        if (iRet != 0)
        {
            PL_LOG_INFO("event loop exit...");
            return false;
        }
        return true;
    }

    bool TcpSrv::Accept()
    {
        struct sockaddr_in clientAddr;
        int iAcceptFd = p_mListenSock->SockAccept((struct sockaddr*)&clientAddr);
        if (iAcceptFd < 0)
        {
            PL_LOG_ERROR("accept new connect fail");
            return false;
        }

        PL_LOG_DEBUG( "accept new client conn, new fd: %d, peer ip: %s, peer port: %d",
                     iAcceptFd, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port) );

        if (!m_pConnReqQueue)
        {
            PL_LOG_ERROR("conn req queue not create");
            close(iAcceptFd);
            iAcceptFd = -1;
            return false;
        }

        std::shared_ptr<ConnNodeType> reqConnNode = std::make_shared<ConnNodeType>(
            iAcceptFd, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), time(NULL));
        PL_LOG_DEBUG("put conn node to queue, node: [ %s ]", reqConnNode->ToString().c_str());

        m_pConnReqQueue->Put(reqConnNode);
        m_iAcceptCountNow++;
        return true;
    }

    bool TcpSrv::RegisteNotifyTimer()
    {
        if (pEv_Tmr == NULL)
        {
            pEv_Tmr = event_new(m_pEventBase, -1, EV_PERSIST,
                                NotifyTimeOutCallback, this); 
            if (pEv_Tmr == NULL)
            {
                PL_LOG_ERROR("new timer event fail");
                return false;
            }

            struct timeval tv;
            evutil_timerclear(&tv);
            tv.tv_sec = m_iTmerNotifyTms;
            evtimer_add(pEv_Tmr, &tv);

            PL_LOG_DEBUG("add notify timer succ, timer tm: %d", m_iTmerNotifyTms);
        }
        return true;
    }

    void TcpSrv::NotifyTimeOutCallback(int fd, short event, void* pHandle)
    {
        PL_LOG_DEBUG("NotifyTimeOutCallback() ");
        TcpSrv* pTcpSrv = (TcpSrv*)pHandle;
        if (pTcpSrv == NULL)
        {
            return ;
        }
        pTcpSrv->NotifyTmoutProcess();
    }

    void TcpSrv::NotifyTmoutProcess()
    {
        PL_LOG_DEBUG("notify timer on doing....");
        
        int iDiff = m_iAcceptCountNow - m_iAcceptCountLast;
        if (iDiff == 0)
        {
            PL_LOG_DEBUG("has not any flow req within %ds, continue", m_iTmerNotifyTms);
            return ;
        }

        PL_LOG_DEBUG("now recv req nums: %d within %ds, notify timer thread",
                     iDiff, m_iTmerNotifyTms);
        
        int iThreadNums = m_pThreadPool->GetThreadPoolNums();
        if (iThreadNums <= 1)
        {
            PL_LOG_ERROR("not extral thread for doing notify");
            return ;
        }

        WorkerTask* pThreadNotify = m_pThreadPool->GetOneThread(iThreadNums-1);
        if (pThreadNotify == NULL)
        {
            PL_LOG_ERROR("get the %d No. thread is null", iThreadNums-1);
            return ;
        }
        
        //timer cmd char: 't'
        if (false == pThreadNotify->NotifySendConn('t'))
        {
            return ;
        }
    
        m_iAcceptCountLast = m_iAcceptCountNow = 0;
        PL_LOG_DEBUG("notify timer cmd send succ");
    }
}
