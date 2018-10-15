/**
 * @file: t_pools.hpp
 * @brief: 线程池容器实现文件. 内部提供收集所有线程就绪的接口。
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-04-22
 */

#ifndef __T_THREAD_POOLS_HPP__
#define __T_THREAD_POOLS_HPP__

#include <pthread.h>
#include <vector>
#include <iostream>

namespace T_TCP
{

    template <typename T, typename V>
    class PthreadPools
    {
     public:
      PthreadPools(const int iPoolSize);
      virtual ~PthreadPools();

      /**
       * @brief: CreateThreads 
       * 创建一序列线程对象, 但此处不启动线程池中的线程
       *
       * @return 
       */
      bool CreateThreads();

      /**
       * @brief: SetThreadContent 
       * 向线程池中的每个线程对象添加私有数据, 线程对象必须实现 SetData(const V& data)方法
       * @param data
       *
       * @return 
       */
      bool SetThreadContent(V& data);

      /**
       * @brief: SetTheadPoolEachThead 
       * 向线程池中每个线程对象添加另外一个消费线程池,
       * 方便前者向后者中的每个线程 分发任务.
       * @param pThreadPool
       *
       * @return 
       */
      bool SetTheadPoolEachThead( PthreadPools<T,V>* pThreadPool );

      /**
       * @brief: StartAllThreads 
       *  启动线程池中所有线程.
       * @return 
       */
      bool StartAllThreads();
        
      /**
       * @brief: AllocateThread 
       *   从线程池中分配一个可用，已经被启动的线程.
       * @return 返回线程的地址.
       */
      T* AllocateThread();
    
      int GetThreadPoolNums()
      {
          return m_iPoolSize;
      }
      /**
       * @brief: GetOneThread 
       * 获取一个特定下标的线程
       *
       * @param iIndex
       *
       * @return 
       */
      T* GetOneThread(int iIndex);
     private:

      /**
       * @brief: WaitThreadRegiste 
       *   等待线程池所有线程都创建完成.
       */
      void WaitThreadRegiste();

      /**
       * @brief: DeleteThreads 
       *  等待线程池中所有线程都退出。
       *  并删除线程对象资源
       */
      void DeleteThreads();
      pthread_mutex_t m_InitLock;
      pthread_cond_t  m_InitCond;
      std::atomic<int> m_iReadyThreadNums;

      std::vector<T*> m_vPoolList; 
      int m_iPoolSize;
      
      pthread_mutex_t m_AllocLock;
      std::atomic<int> m_iCurThreadIndex;
    };

    ////////////////////////////////////////////
    ///implement for pthreadpools
    template<typename T, typename V>
    PthreadPools<T,V>::PthreadPools(const int iPoolSize)
                    : m_iReadyThreadNums(0), m_iPoolSize(iPoolSize), 
                    m_iCurThreadIndex(0)
    {
        pthread_mutex_init(&m_InitLock, NULL);
        pthread_cond_init(&m_InitCond, NULL);

        pthread_mutex_init(&m_AllocLock, NULL);
    }

    template<typename T, typename V>
    PthreadPools<T,V>::~PthreadPools ()
    {
        DeleteThreads();
    }

    template<typename T, typename V>
    bool PthreadPools<T,V>::CreateThreads()
    {
        if (m_iPoolSize <= 0)
        {
            return false;
        }
        
        for (int i = 0; i < m_iPoolSize; ++i)
        {
            //last thread used for manager cmd process
            T* pThread = new T(&m_InitLock, &m_InitCond, &m_iReadyThreadNums, i);
            m_vPoolList.push_back(pThread);
        }
        return true;
    }

    template<typename T, typename V>
    bool PthreadPools<T,V>::SetThreadContent(V& data)
    {
        for( auto& one: m_vPoolList )
        {
            one->SetData(data);
        }
        return true;
    }

    template<typename T, typename V>
    bool PthreadPools<T,V>::SetTheadPoolEachThead(PthreadPools<T,V>* pThreadPool)
    {
        for( auto& one: m_vPoolList )
        {
            one->SetThreadPool(pThreadPool);
        }
        return true;

    }

    template<typename T, typename V>
    bool PthreadPools<T,V>::StartAllThreads()
    {
        for (int i = 0; i < m_iPoolSize; ++i)
        {
            m_vPoolList.at(i)->Start();
        }

        WaitThreadRegiste();
        PL_LOG_INFO( "all threads start ok, worker threads nums: %d", m_iPoolSize );
        return true;
    }

    
    template<typename T, typename V>
    void PthreadPools<T,V>::WaitThreadRegiste()
    { 
        pthread_mutex_lock(&m_InitLock);
        while( m_iReadyThreadNums < m_iPoolSize)
        {
            pthread_cond_wait(&m_InitCond, &m_InitLock);
        }
        pthread_mutex_unlock(&m_InitLock);
    }

    template<typename T, typename V>
    void PthreadPools<T,V>::DeleteThreads()
    {
        if (m_iPoolSize <=0)
        {
            return ;
        }

        for (int i = 0; i < m_iPoolSize; ++i)
        {
            m_vPoolList.at(i)->JoinWork();
            delete m_vPoolList[i]; 
        }
        m_vPoolList.clear();
    }

    template<typename T, typename V>
    T* PthreadPools<T,V>::AllocateThread()
    {
        int iIndex = (m_iCurThreadIndex++) % (m_iPoolSize - 1); //last thread to timer worker
        PL_LOG_DEBUG("get work thread index: %d", iIndex);
        return m_vPoolList.at(iIndex);
    }

    template<typename T, typename V>
    T* PthreadPools<T,V>::GetOneThread(int iIndex)
    {
        if (iIndex >= m_iPoolSize || iIndex < 0)
        {
            return  NULL;
        }
        return m_vPoolList.at(iIndex);
    }
}

#endif
