#ifndef _WORKPTHREAD_H_
#define _WORKPTHREAD_H_

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <string.h>
#include <vector>
#include <list>
#include <queue>
#include <deque>
#include <map>
#include <set>
 
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
 
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 
#include <errno.h>
#include <utility>
#include <fcntl.h>
#include <unistd.h>
#include "avs_mts_log.h"

#include <mutex>
#include <thread>

// 工作线程的实现
class WorkPthread
{
public:
	WorkPthread();
	virtual ~WorkPthread();
	void start();			//线程的启动
	virtual int run() = 0;
	void  workPthreadFunc();

protected:
	string m_name;
    std::thread m_Thread;
    std::mutex m_Mutex;
};


#endif
