/**
 * @file: t_tcpsrv.hpp
 * @brief:  tcp 服务的主进程(主线程), 期间listenconn的连接对象向其
 * 注册，由这些连接对象来监控网络事件，具体网络事件的逻辑处理交给主线程
 * 。 主进程也分配一个连接线程池，当有新tcp连接到达时，从线程池中分配一
 * 个线程用于数据的收发(一般通过管道方式转移新的连接,  acceptconn 连接对象向工作
 * 线程注册网络事件,由该对象来监控新线程上的网络事件，事件就绪后交给工作线程处理)。
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-04-23
 */

#ifndef _POLICY_SERVER_H_
#define _POLICY_SERVER_H_

#include  <string>
#include  "t_socket.hpp"
#include  "t_worktask.hpp"
#include  "t_pools.hpp"
#include  <event2/event.h>
#include  <event2/buffer.h>
#include  <event.h>
#include  "policy_log.h"
#include  "redis_conn_pool.h"
#include  "redis_pool.h"
#include  "FreeLockQueue.h"
#include  "configXml.h"
#include  "LockQueue.h"
#include "t_req_conn_node.h"

#include <memory>

#define ConnectFreeListLen     (128)

using namespace util;
namespace T_TCP
{
    class ListenConn;
    class TcpSrv
    {
     public:
      TcpSrv(const std::string& sIp, int iPort,int ithreadNums);
      virtual ~TcpSrv();
      bool Run();
      
      bool Accept();
     private:
      bool RegisteAcceptConn();
      bool Init();
      bool LoadNodeLoadFromDB();

      bool RegisteNotifyTimer();

      static void NotifyTimeOutCallback( int fd, short event, void* pHandle);
      
      void NotifyTmoutProcess();
     private:
      bool m_bInit;
      Sock* p_mListenSock;
      
      std::string  m_sLocalIp;
      int m_iLocalPort;
      struct event_base* m_pEventBase;
      
      //thread pools: task thread pool obj
      PthreadPools<WorkerTask, std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType>>>>* m_pThreadPool;

      //thread pools: dispatch new conn thread pool obj
      PthreadPools<WorkerTask, std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType>>>>* m_pDispathConnThreadPool;

      int m_iThreadPoolNums;
      
      FreeLockQue<QueueItem>* m_pAcceptConnListWorking;
      FreeLockQue<QueueItem>* m_pAcceptConnListFree;
     
      ListenConn* p_mListenConn; //in future, it can been allocated by conn_pools

      std::shared_ptr<FS_RedisInterface> m_pRedisHandle;

      //增加统计接收请求的个数
      volatile int m_iAcceptCountNow;
      volatile int m_iAcceptCountLast;
      int m_iTmerNotifyTms;  //定时通知落库线程开始落库的时间间隔,unit is second
      struct event* pEv_Tmr;
      //
      int m_iConnReqQueueLen;
      std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType>>> m_pConnReqQueue; //用于存储peer端的连接
    };
}
#endif


