/**
 * @file: LockQueue.h
 * @brief: 定一个fifo队列，用于生产者-消费者间消息传递
 * 使用方式:
 *  LockQueue que(100)
 *  Item in_item;
 *  que.Put(in_item);
 *
 *  Item out_item;
 *  que.Get(&out_item);
 *
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-10-14
 */


#ifndef _LOCK_QUEUE_H_
#define _LOCK_QUEUE_H_

#include <mutex>
#include <condition_variable>
#include <list>

namespace util 
{

template<typename T>
class LockQueue
{
 public:
    LockQueue(int iQSize = 512):m_iQueueSize(iQSize) {}
    virtual ~LockQueue() {}

    void Get(T* item)
    {
        std::unique_lock<std::mutex> lock(m_MutexLock);
        while (m_QList.empty())
        {
            m_GetCond.wait(lock);
        }
        *item = m_QList.front();
        m_QList.pop_front();

        m_SetCond.notify_one();
    }
    
    void Put(const T& item)
    {
        std::unique_lock<std::mutex> lock(m_MutexLock);
        while (m_QList.size() == m_iQueueSize)
        {
            m_SetCond.wait(lock);
        }
        m_QList.push_back(item);

        m_GetCond.notify_one();
    }

    int Size() const
    {
        std::lock_guard<std::mutex> lock(m_MutexLock);
        return m_QList.size();
    }

    bool IsFull() const
    {
        return Size() == m_iQueueSize;
    }

    bool IsEmpty() const
    {
        return Size() == 0;
    }

 private:
    LockQueue(const LockQueue& item);
    LockQueue& operator=(const LockQueue& item);
    //
    std::mutex              m_MutexLock;
    std::condition_variable m_SetCond;
    std::condition_variable m_GetCond;
    std::list<T>            m_QList;
    unsigned int            m_iQueueSize;
};

}
#endif
