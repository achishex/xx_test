/**
 * @file: NotifyPrivateMesage.hxx
 *
 * @brief:  define interface for nofity to invite process
 *
 * @author:  wusheng Hu
 * @version: v0X00001
 * @date: 2018-09-12
 */

#if !defined(NOTIFYPRIVATEMESAGE_HXX)
#define NOTIFYPRIVATEMESAGE_HXX

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"

#include "AsyncProcessorMessage.hxx"
#include "RequestContext.hxx"
#include "Proxy.hxx"

#include "resip/stack/SipStack.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "resip/stack/ExtensionHeader.hxx"
#include "resip/dum/ContactInstanceRecord.hxx"

#include "RequestContext.hxx"
#include "AsyncProcessor.hxx"

#include <sys/time.h>
#include <map>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

namespace repro
{
    typedef struct _MediaPortSource
    {
        short int relay_caller_audio_rtp_port;  //呼叫音频RTP端口
        short int relay_caller_audio_rtcp_port; //呼叫音频RTCP端口

        short int relay_caller_video_rtp_port;  //呼叫视频RTP端口
        short int relay_caller_video_rtcp_port; //呼叫视频RTCP端口


        short int relay_callee_audo_io_rtp_port;//被呼叫音频RTP端口
        short int relay_callee_audo_io_rtcp_port;//被呼叫音频RTCP端口

        short int relay_callee_video_io_rtp_port;//被呼叫视频RTP端口
        short int relay_callee_video_io_rtcp_port;//被呼叫视频RTCP端口

        std::string m_MtsIp;                    //mts ip
        short int   m_mtsPort;                  //mts port

        bool m_CallerTcpTransport;              //caller transport tcp
        bool m_CalleeTcpTransport;              //callee transport tcp

        _MediaPortSource()
        {
            relay_caller_audio_rtp_port     =0;
            relay_caller_audio_rtcp_port    =0;
            relay_caller_video_rtp_port     =0;
            relay_caller_video_rtcp_port    =0;
            relay_callee_audo_io_rtp_port   =0;
            relay_callee_audo_io_rtcp_port  =0;
            relay_callee_video_io_rtp_port  =0;
            relay_callee_video_io_rtcp_port =0;

            m_MtsIp.clear(); 
            m_mtsPort = 0;

            m_CallerTcpTransport = m_CalleeTcpTransport = false; //default transport is udp
            DebugLog( << "deconstruct media port info struct" );
        }

        _MediaPortSource(const _MediaPortSource& item)
        {
            relay_caller_audio_rtp_port     =item.relay_caller_audio_rtp_port;
            relay_caller_audio_rtcp_port    =item.relay_caller_audio_rtcp_port;
            relay_caller_video_rtp_port     =item.relay_caller_video_rtp_port;
            relay_caller_video_rtcp_port    =item.relay_caller_video_rtcp_port;
            relay_callee_audo_io_rtp_port   =item.relay_callee_audo_io_rtp_port;
            relay_callee_audo_io_rtcp_port  =item.relay_callee_audo_io_rtcp_port;
            relay_callee_video_io_rtp_port  =item.relay_callee_video_io_rtp_port;
            relay_callee_video_io_rtcp_port =item.relay_callee_video_io_rtcp_port;
            m_MtsIp                         =item.m_MtsIp;
            m_mtsPort                       =item.m_mtsPort;
            m_CallerTcpTransport            =item.m_CallerTcpTransport;
            m_CalleeTcpTransport            =item.m_CalleeTcpTransport;
        }

        friend std::ostream& operator<<(std::ostream& strm, struct _MediaPortSource& info)
        {
            char buf[1024];
            int iLen = snprintf(buf,sizeof(buf),"[ mts_ip, mts_port, caller_a_rtp_port, caller_a_rtcp_port,"
                     "caller_v_rtp_port, caller_v_rtcp_port, callee_a_rtp_port, callee_a_rtcp_port,"
                     "callee_v_rtp_port,callee_v_rtcp_port, caller_tcp_transport, callee_tcp_transport]"
                     " => [%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",
                     info.m_MtsIp.c_str(), info.m_mtsPort,
                     info.relay_caller_audio_rtp_port,
                     info.relay_caller_audio_rtcp_port,
                     info.relay_caller_video_rtp_port,
                     info.relay_caller_video_rtcp_port,
                     info.relay_callee_audo_io_rtp_port,
                     info.relay_callee_audo_io_rtcp_port,
                     info.relay_callee_video_io_rtp_port,
                     info.relay_callee_video_io_rtcp_port,
                     info.m_CallerTcpTransport,
                     info.m_CalleeTcpTransport);

            return strm.write( buf, iLen);
        }
    } MediaPortSource;

//define mesg contents
class NotifyMessage
{
 public:
  NotifyMessage() {}
  ~NotifyMessage() {}
  //
  resip::ContactList vdstContact;
};

//define class for notify sending msg to inviter
class NotifyToInviteChannel
{
 public:
  NotifyToInviteChannel(RequestContext& reqContext, AsyncProcessor* asyncproc);
  virtual ~NotifyToInviteChannel();

  bool Post(const NotifyMessage& msg);
 private:
  RequestContext& m_ReqContext;
  AsyncProcessor* m_pAsyncProccor;
};

}
#endif
