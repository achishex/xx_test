#include "workPthread.h"

WorkPthread::WorkPthread()
{
}

void WorkPthread::start()
{
    m_Thread = std::thread(&WorkPthread::workPthreadFunc, this);
}

void WorkPthread::workPthreadFunc()
{
    this->run();
}


WorkPthread::~WorkPthread()
{
    m_Thread.join();
}
