/**
 * @file: t_worktask.hpp
 * @brief: 已建立连接的工作线程，其主要能力包括：
 *       提供接口使其他线程向其发送命令(一般提供管道的写fd), 
 *       管道连接对象向其注册，管道对象只检测管道上的读事件， 事件就绪后 交回给
 *       工作线程处理。
 *       accepted的连接对象向其注册，该对象只负责监控连接句柄上的读写事件，事件
 *       就绪接收和发送，协议编解码, 逻辑处理等。该工作线程总体上起事件调度能力
 *       (如果accept连接对象后续逻辑处理复杂了，可以拆分出来，转交给工作线程去处
 *       理，工作线程可以转移给他线程来做)
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-04-23
 */

#ifndef _t_worktask_hpp_
#define _t_worktask_hpp_

#include  "FreeLockQueue.h"
#include  "t_queue.hpp"
#include  "t_thread.hpp"
#include  "t_eventbase.hpp"
#include  "policy_log.h"
#include  <pthread.h>

#include  <unistd.h>
#include  <string.h>
#include  <map>
 
#include  <event2/event.h>
#include  <event2/buffer.h>
#include  <event.h>

#include  "LockQueue.h"
#include "t_req_conn_node.h"
#include "t_pools.hpp"

using namespace util;

namespace T_TCP
{
    class PipeConn;
    class WorkerTask: public PthreadBase
    {
     public:
      WorkerTask();
      WorkerTask(pthread_mutex_t* pInitLock, pthread_cond_t* pInitCond, std::atomic<int>* pInitNums, int iWorkId);
      virtual ~WorkerTask();

      /**
       * @brief: SetData 
       * 在分发线程池中的每个线程上保存 连接连接队列，分发线程作为消费者取出连接节点，向工作
       * 线程转发该连接
       * @param QConnNode
       */
      void SetData( std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType> > >& QConnNode );

      /**
       * @brief: SetThreadPool 
       * 在分发线程池中保存实际业务线程池，前者中的每个线程在取到节点后向后者派
       * 发
       * @param pThreadPool
       */
      void SetThreadPool( PthreadPools<WorkerTask, std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType>>>>* pThreadPool );

      int GetWorkId() const 
      {
          return m_iWorkId;
      }

      struct event_base *GetEventBase() { return m_PthreadEventBase; }
      
     /**
      * @brief: AddRConn 
      *  把读数据的连接对象注册到主线程上.该连接对象已经提供读写事件到达时的回
      *  调接口。主线程只负责调度
      * @param pEvent
      */
     void AddRConn(ConnBase* pEvent);
    
     /**
      * @brief: NotifyRecvConn
      *     从管道上接收命令字后，对命令字的解析和处理. 用于PipeConn对象接收命令, 在pipe连接上有网络事件
      *     到达后，事件回调函数处理.
      * @param pBuf
      *   具体命令字空间
      * @param iLen
      *   具体命令字的长度
      *
      * @return 
      */
     bool NotifyRecvConn(char *pBuf, int iLen);

     /**
      * @brief: NotifySendConn 
      *     向发送命令管道上命令字，并保存命令字列表队列
      *     该接口用于向任务线程的发送管道上发送命令字, 对外被[其他]非任务线程调用.
      * @param pItemList
      *   已经发送的命令字列表队列.
      *
      * @return 
      */
     bool NotifySendConn(FreeLockQue<QueueItem>* pItemList, FreeLockQue<QueueItem>* pItemListFree);
     bool NotifySendConn(char cmdChar);

     /**
      * @brief: NotifySendConn 
      * 直接发送fd
      * @param pBuf
      * @param iLen
      *
      * @return 
      */
     bool NotifySendConn(const char *pBuf, int iLen);

     /**
      * @brief: DeleteAcceptConn 
      *     删除本任务线程上的已经连接的连接对象，关闭该连接socket.
      * @param fd
      */
     void DeleteAcceptConn(int fd);
    protected:
     virtual int main();
     virtual int Init();

    private:
     void FlushNodeLoadToDb();

     void MainLoop();
    
    private:
     int m_iNotifyRecvFd;
     int m_iNofitySendFd;

     struct event_base *m_PthreadEventBase;
     FreeLockQue<QueueItem>* m_pConnItemList;
     FreeLockQue<QueueItem>* m_pConnItemListFree;

     PipeConn* m_pPipNotifyConn;
     //
     std::map<int, ConnBase*> m_mpAcceptConn; //need not lock
     int m_iWorkId;
     std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType>>> m_reqConnQueue;
     PthreadPools<WorkerTask, std::shared_ptr<LockQueue<std::shared_ptr<ConnNodeType>>>>* m_pThreadPool;
    };
}

#endif 
