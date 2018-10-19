#ifndef _UDPSESSION_H_ 
#define _UDPSESSION_H_

#include <vector>
#include <list>
#include <string>
#include <atomic>

#include <string.h>
#include <unistd.h>
#include <event2/event.h>

#include <unistd.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event.h>
#include <memory>
#include <thread>

#include "rapidjson/writer.h"                                                                                           
#include "rapidjson/stringbuffer.h"                                                                                     
#include "rapidjson/document.h"                                                                                         
#include "rapidjson/error/en.h"                                                                                         
#include "rapidjson/prettywriter.h" 

#include "t_PortPools.h"
#include "t_tcp_client.h"

struct SessionItem
{
    int             m_port;       //端口
    int             m_sock;       //socket描述符
    struct event    m_event;       // event
    
    volatile atomic<bool>   m_is_data_ok;   //

    int             m_peer_sock;    //
    std::string     m_session_id;   // 
    struct sockaddr_in  m_remoteAddr;
    int             m_iRecvPkgNums;

    SessionItem(): m_port(0), m_sock(-1), m_is_data_ok(false),
                    m_peer_sock(-1), m_session_id(""), m_iRecvPkgNums(0)
    { 
        ::bzero(&m_remoteAddr, sizeof(m_remoteAddr)); 
        memset(&m_event, 0, sizeof(m_event));
    }

    const std::string ToString() const
    {
        std::stringstream ios;
        ios << " [port, sock, recv_pkg_nums, sess_id, is_recv_data_flag] => [ "
            << m_port << "," << m_sock << "," << m_iRecvPkgNums << "," 
            << m_session_id << "," << m_is_data_ok << " ]";
        
        std::string dstStr(ios.str());
        return dstStr;
    }

    virtual ~SessionItem();
 private:
    SessionItem(const SessionItem& item);
    SessionItem& operator = (const SessionItem& item);
};

class UdpSession
{
private:
    static const int BUF_MAX_SIZE = 4096 * 10;
public:
    UdpSession();
   ~UdpSession();
    bool Init(const RelayPortRecord& record, struct event_base * base,
              const std::string& sMediaIp, unsigned int uiMediaPort);

   void ResetResource();
    void NotifyCloseSession( ); // 开始 退出
private:
    static void recv_callback(int fd, short event, void* arg) ;
    static void timer_callback(int fd, short event, void* arg);

    void ReadProcess( int fd,  short event ) ;
    void TimerTimeOutCallback( int fd,  short event );

    void CloseSession();
    int CreateUdpEvent( const std::string& sessid, 
                        unsigned short usPort,
                        struct event_base * base);
   bool UpdatePeerFd(int irstFd, int indFd);
   std::string ToString();
   void ReleaseMtsPort( );

   bool BuildReleaseMtsMsg( rapidjson::Document& onRoot );
private:
   // 是否要退出
   std::atomic<bool>            m_destroying;  
   std::string                  m_sessionId;
   //key: fd
   std::map<int, std::shared_ptr<SessionItem>> m_mpSessionPorts; 
   //add monitor timer
   struct event*                m_pMoniterTimeEvent;
   int                          m_iCountAccum;
   std::string                  m_sMediaIp;
   int                          m_iMediaPort;
   std::shared_ptr<TcpClient>   m_pReleasePortHandle;

   static int                   MAXTIMES_FAIL;      // 统计端口连续未收到数据的次数
   static int                   PortMoniterTimerTm; // 监控端口的定时器时间
};



class EventManager: public CSingleton<EventManager>
{
public:
    UdpSession*  acquire() ;  // 获取
    void  release( UdpSession* e ) ; // 释放
    void Init(event_base * base);
    void Init(int iQSize = 30, int iCheckTms = 300 /** 5minute **/);
    virtual ~EventManager();
 private:
    EventManager()
        :m_base(NULL), m_iSessionSize(30),
        m_iCheckTm(300), m_ThreadStop(false) {}
    void CheckTimerTmoutCallback();
    
    std::mutex                  m_lock;
    std::list< UdpSession* >    m_list ;
    event_base*                 m_base;

    int                         m_iSessionSize;
    int                         m_iCheckTm;

    std::shared_ptr<std::thread>    m_pthdCheckTimer;
    std::atomic<bool>               m_ThreadStop;
    //
    friend CSingleton<EventManager>;
};

#endif
