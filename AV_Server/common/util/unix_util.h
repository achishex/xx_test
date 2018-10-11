/**
 * @file: unix_util.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: V0001
 * @date: 2018-04-10
 */

#ifndef __UNIX_UTIL_H__
#define __UNIX_UTIL_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <sys/resource.h>
#include <string.h>
#include <stdint.h>

#include <iostream>

typedef void (*mts_sig_handler)(int);
namespace Util 
{
    /**
     * @brief: InstallSignal
     *  设置忽略的信号
     */
    void InstallSignal(mts_sig_handler pCallback);

    /**
     * @brief: daemonize 
     *   设置后台守护进程
     * @param pCmd: 进程名
     * @param nochdir, 是否需要将进程的路劲设置为 "/";  大于0 标示 不用把路径改变为"/"
     *      if we want to ensure our ability to dump core, don't chdir to / *
     * @param noclose, 是否关闭标准的输入，输出。 大于0 标示不关闭。小于0 需要把标注输入输出重新定向.
     */
    void daemonize(const char* pCmd, int nochdir = 1, int noclose = 0);   
    int  SetOpenFileNums(const int iMaxOpenFilesNums = 65535);
    int SetCoreMax();
    
    class ProcTitle
    {
        public:
         ProcTitle(int argc, char **argv);
         virtual ~ProcTitle();
         /**
          * @brief: SetTitle
          *   修改进程名, 通过ps 查看
          * @param pTitle
          *   指定的进程名字符串 
          */
         void SetTitle(const char* pTitle);
         /**
          * @brief: GetTitle
          *  获取当前已被设置的进程名字符串
          * @return 
          *  被设置的进程名字符串
          */
         const char* GetTitle();
        private:
         /**
          * @brief: Init 
          *  利用系统当前环境初始化进程title设置操作
          *
          * @return false: 失败, true: 成功
          */
         bool Init();
        private:
         char  *m_pOsArgvLast; 
         int32_t m_iArgBufSize; 
         char ** m_pArgv; 
         bool m_bInit;
         char* m_pEnv;
    };
}
#endif
