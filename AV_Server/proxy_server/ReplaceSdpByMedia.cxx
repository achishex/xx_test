#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <iostream>
#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Inserter.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"
#include "Proxy.hxx"
#include "ResponseContext.hxx"
#include "RequestContext.hxx"
#include "RRDecorator.hxx"
#include "Ack200DoneMessage.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/WinLeakCheck.hxx"

#include  "ReplaceSdpByMedia.hxx"


using namespace resip;
using namespace repro;
using namespace std;

typedef std::list<SdpContents::Session::Connection>::iterator Iter_Conn;

SdpReplaceMedia::SdpReplaceMedia(Contents* pCntext, MediaPortSource* pMedia, bool req )
    :m_pCntents(pCntext), m_pMedia(pMedia), m_isReq(req)
{

}

SdpReplaceMedia::~SdpReplaceMedia()
{

}

bool SdpReplaceMedia::Replace()
{
    if (!m_pCntents || !m_pMedia)
    {
        ErrLog( << "contents or mediaportsource handle is null" );
        return false;
    }

    SdpContents* psdpContents = dynamic_cast<SdpContents*>(m_pCntents);
    if ( psdpContents == NULL )
    {
        ErrLog( << "contents is not sdp content" );
        return false;
    }
    SdpContents::Session& session = psdpContents->session();
    SdpContents::Session::Origin& val_origin = session.origin();
    val_origin.setAddress(m_pMedia->m_MtsIp.c_str());
    DebugLog( << "replace media ip: " << m_pMedia->m_MtsIp ); 

    typedef  SdpContents::Session::MediumContainer::iterator  Iter_Media_List;
    SdpContents::Session::MediumContainer& v_media = session.media();

    for ( Iter_Media_List it = v_media.begin(); it !=  v_media.end(); ++it )
    {
        if (it->name() == "audio" )
        {
            if ( SetAudioMediaLine(*it) == false )
            {
                ErrLog( << "replace MediaLine audio fail" );
                continue;
            }
        }
        else if (it->name() == "video" )
        {
            if ( SetVideoMediaLine(*it) == false )
            {
                ErrLog( << "replace MediaLine video fail" );
                continue;
            }
        }
        else 
        {
            ErrLog( << "m line, name: " << it->name() );
            continue;
        }
    }

    return true;
}

bool SdpReplaceMedia::SetAudioMediaLine( SdpContents::Session::Medium& media )
{
    return SetMediaLine(media);
}

bool SdpReplaceMedia::SetVideoMediaLine( SdpContents::Session::Medium& media )
{
    return SetMediaLine(media);
}

bool SdpReplaceMedia::SetMediaLine( SdpContents::Session::Medium& media )
{
    unsigned int i_rtpPort = 0;
    if (m_isReq == true)
    {
        //set caller rtp port
        DebugLog (<<"this is invite sdp"); 
        if ( media.name() == "audio" )
        {
            i_rtpPort = m_pMedia->relay_caller_audio_rtp_port;
        }
        else if ( media.name() == "video" )
        {
            i_rtpPort = m_pMedia->relay_caller_video_rtp_port;
        }
    }
    else 
    {
        //set callee rtp port
        if ( media.name() == "audio" )
        {
            i_rtpPort = m_pMedia->relay_callee_audo_io_rtp_port;
        }
        else if (media.name() == "video")
        {
            i_rtpPort = m_pMedia->relay_callee_video_io_rtp_port;
        }
        DebugLog( << "this is 200ok sdp" );
    }
    media.setPort(i_rtpPort);
   
    std::list<SdpContents::Session::Connection>& conn_list = media.getMediumConnections();
    for (Iter_Conn it_con = conn_list.begin(); it_con != conn_list.end(); ++it_con)
    {
        it_con->setAddress(m_pMedia->m_MtsIp.c_str());
    }
    
    if ( false == ProcAttrRtcp(media) )
    {
        ErrLog( << "replace rtcp port fail" );
        return false;
    }

   return true;
}

bool SdpReplaceMedia::ProcAttrRtcp( SdpContents::Session::Medium& media )
{
    if (media.exists("rtcp") == false)
    {
        ErrLog( << "m line not exist rtcp attr " );
        return false;
    }
    if (m_pMedia->m_MtsIp.empty())
    {
        ErrLog( <<"mts ip is empty, check process: get mts ip " );
        return false;
    }

    std::list<Data> rtcp_val_list( media.getValues("rtcp") );
    DebugLog( << "rtcp val list size: " << rtcp_val_list.size() );  

    if (rtcp_val_list.empty() == true)
    {
        ErrLog( << "rtcp attr value is empty" );
        return false;
    }

    const Data& front_data = rtcp_val_list.front();
    DebugLog( << "rtcp attr first value: " << front_data );
    rtcp_val_list.pop_front();

    unsigned int i_rtcpPort = 0;
    if (m_isReq == true)
    {
        DebugLog (<<"this is invite sdp"); 
        if ( media.name() == "audio" )
        {
            i_rtcpPort = m_pMedia->relay_caller_audio_rtcp_port;
        }
        else if (media.name() == "video")
        {
            i_rtcpPort = m_pMedia->relay_caller_video_rtcp_port;
        }
        else
        {
        }
    }
    else 
    {
        DebugLog( << "this is 200ok sdp" );
        if (media.name() == "audio")
        {
            i_rtcpPort = m_pMedia->relay_callee_audo_io_rtcp_port;
        }
        else if (media.name() == "video")
        {
            i_rtcpPort = m_pMedia->relay_callee_video_io_rtcp_port;
        }
        else
        {
        }
    }

    char buf[128];
    memset(buf,0, sizeof(buf));
    snprintf(buf,sizeof(buf), "%d IN IP4 %s", i_rtcpPort, m_pMedia->m_MtsIp.c_str());
    rtcp_val_list.push_front(buf);

    Data rtcp_val;
    for (auto &one: rtcp_val_list)
    {
        rtcp_val += (one);
    }

    media.clearAttribute("rtcp");
    media.addAttribute("rtcp", rtcp_val);

    return true;
}

