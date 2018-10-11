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

#include "t_PortPools.h"

struct SessionItem
{
    int             m_port;       //端口
    int             m_sock;       //socket描述符
    struct event    m_event;       // event
    
    volatile atomic<bool>   m_is_data_ok;   //

    int             m_peer_sock;    //
    std::string     m_session_id;   // 
    struct sockaddr_in  m_remoteAddr;

    SessionItem(): m_port(0), m_sock(-1), m_is_data_ok(false),
                    m_peer_sock(-1), m_session_id("")
    { 
        ::bzero(&m_remoteAddr, sizeof(m_remoteAddr)); 
        memset(&m_event, 0, sizeof(m_event));
    }

    const std::string ToString() const
    {
        std::stringstream ios;
        ios << " [port, sock, sess_id, is_recv_data_flag] => [ "
            << m_port << "," << m_sock << "," << m_session_id << ","
            << m_is_data_ok << " ]";
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
    bool Init(const RelayPortRecord& record, struct event_base * base );

    void NotifyCloseSession( ); // 开始 退出
private:
    static void recv_callback(int fd, short event, void* arg) ;
    void ReadProcess( int fd,  short event ) ;

    void CloseSession();
    int CreateUdpEvent( const std::string& sessid, 
                        unsigned short usPort,
                        struct event_base * base);
   bool UpdatePeerFd(int irstFd, int indFd);
   std::string ToString();
private:
   // 是否要退出
   std::atomic<bool>            m_destroying;  
   std::string                  m_sessionId;
   //key: fd
   std::map<int, std::shared_ptr<SessionItem>> m_mpSessionPorts; 
};



class EventManager: public CSingleton<EventManager>
{
public:
    UdpSession*  acquire() ;  // 获取
    void  release( UdpSession* e ) ; // 释放
    void Init(event_base * base);
    EventManager();

 private:
    std::mutex      m_lock;
    std::list< UdpSession* >  m_list ;
    event_base * m_base;
};

#endif
