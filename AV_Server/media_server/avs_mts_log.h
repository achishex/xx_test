#ifndef _AVS_MTS_LOG_H_
#define _AVS_MTS_LOG_H_

#include "log.h"

#define MTS_AUDIO_BUF_SIZE  1024*8

#define MTS_LOG_INFO(args, ...) do { \
    char buffer[MTS_AUDIO_BUF_SIZE]; \
    snprintf(buffer,FMT_LEN,"[ %s %s (%d) ]: %s",__FILE__, __FUNCTION__, __LINE__,args); \
    if (pAvsLog) { \
        pAvsLog->info(buffer,##__VA_ARGS__);\
    }\
} while(0)

#define  MTS_LOG_ERROR(args, ...) do { \
    char buffer[MTS_AUDIO_BUF_SIZE]; \
    snprintf(buffer,FMT_LEN,"[ %s %s (%d) ]: %s",__FILE__, __FUNCTION__, __LINE__,args); \
    if (pAvsLog) { \
        pAvsLog->error(buffer,##__VA_ARGS__);\
    }\
}while(0)

#define MTS_LOG_DEBUG(args, ...) do { \
    char buffer[MTS_AUDIO_BUF_SIZE]; \
    snprintf(buffer,FMT_LEN,"[ %s %s (%d) ]: %s",__FILE__, __FUNCTION__, __LINE__,args); \
    if (pAvsLog) { \
        pAvsLog->debug(buffer,##__VA_ARGS__);\
    } \
}while(0)

#define MTS_LOG_NOTICE(args, ...) do { \
    char buffer[MTS_AUDIO_BUF_SIZE]; \
    snprintf(buffer,FMT_LEN,"[ %s %s (%d) ]: %s",__FILE__, __FUNCTION__, __LINE__,args); \
    if (pAvsLog) { \
        pAvsLog->notice(buffer,##__VA_ARGS__);\
    }\
}while(0)


#endif
