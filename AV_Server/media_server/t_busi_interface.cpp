#include <iostream>
#include "t_busi_interface.h"
#include "t_mts_udp_proto.h"

OpenChannel::OpenChannel(const std::string& sMethod, MtsTcp* pMts )
    : PolicyBusi(sMethod, pMts)
{ }

OpenChannel::~OpenChannel()
{ }

bool OpenChannel::ProcessBusi(const rapidjson::Document &inRoot,
                                    rapidjson::Document &oRoot)
{
    ParseCommParams(inRoot);
    if (!m_hasParams)
    {
        MTS_LOG_ERROR("has not params in req json");
        m_stResp.iCode = -1;
        m_stResp.sErrMsg = "req params not include";
        
        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    }

    const rapidjson::Value& paramVal = inRoot["params"];
    std::string sSessionId;

    do {
        if ( paramVal.HasMember("session_id") && paramVal["session_id"].IsString() )
        {
            sSessionId = paramVal["session_id"].GetString();
            MTS_LOG_DEBUG("method: %s, session id: %s", GetMethod().c_str(), sSessionId.c_str());
            break;
        }

        MTS_LOG_ERROR("not has session_id field on method: %s", GetMethod().c_str());
        
        m_stResp.iCode = -1;
        m_stResp.sErrMsg = "has not session_id in params obj";

        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    } while(0);

    //
    auto oneSession = p_mtsTcp->GetPortPool()->m_mpClientSession.find(sSessionId);
    if ( oneSession != p_mtsTcp->GetPortPool()->m_mpClientSession.end() )
    {
        MTS_LOG_ERROR("session has exist, session_id: %s", sSessionId.c_str());
        m_stResp.iCode = -1;
        m_stResp.sErrMsg = "session has exist";
        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    }
    
    RelayPortRecord oneRelaySession;
    
    memcpy(oneRelaySession.m_sSessId, sSessionId.c_str(), sizeof(oneRelaySession.m_sSessId));
    auto pairRtpRtcp = p_mtsTcp->GetPortPool()->GetAndCheckUdpRtpRtcpPort();
    
    oneRelaySession.usCallerARtpPort   = pairRtpRtcp.first;
    oneRelaySession.usCallerARtcpPort  = pairRtpRtcp.second;

    pairRtpRtcp = p_mtsTcp->GetPortPool()->GetAndCheckUdpRtpRtcpPort();
    oneRelaySession.usCallerVRtpPort   =  pairRtpRtcp.first;
    oneRelaySession.usCallerVRtcpPort  = pairRtpRtcp.second;

    //
    pairRtpRtcp = p_mtsTcp->GetPortPool()->GetAndCheckUdpRtpRtcpPort();
    oneRelaySession.usCalleeARtpPort   = pairRtpRtcp.first;
    oneRelaySession.usCalleeARtcpPort  = pairRtpRtcp.second;

    pairRtpRtcp = p_mtsTcp->GetPortPool()->GetAndCheckUdpRtpRtcpPort();
    oneRelaySession.usCalleeVRtpPort   = pairRtpRtcp.first;
    oneRelaySession.usCalleeVRtcpPort  = pairRtpRtcp.second;

    if (oneRelaySession.usCallerARtpPort    <= 0 || oneRelaySession.usCallerARtcpPort <= 0
        || oneRelaySession.usCallerVRtpPort <= 0 || oneRelaySession.usCallerVRtcpPort <= 0
        || oneRelaySession.usCalleeARtpPort <= 0 || oneRelaySession.usCalleeARtcpPort <= 0
        || oneRelaySession.usCalleeVRtpPort <= 0 || oneRelaySession.usCalleeVRtcpPort <= 0)
    {
        std::pair<unsigned short,unsigned short> pairPort;
       
        pairPort.first = oneRelaySession.usCallerARtpPort;
        pairPort.second = oneRelaySession.usCallerARtcpPort;
        p_mtsTcp->GetPortPool()->RecycleUdpPort(pairPort);

        pairPort.first = oneRelaySession.usCallerVRtpPort;
        pairPort.second = oneRelaySession.usCallerVRtcpPort;
        p_mtsTcp->GetPortPool()->RecycleUdpPort(pairPort);

        //
        pairPort.first = oneRelaySession.usCalleeARtpPort;
        pairPort.second = oneRelaySession.usCalleeARtcpPort;
        p_mtsTcp->GetPortPool()->RecycleUdpPort(pairPort);

        pairPort.first = oneRelaySession.usCalleeVRtpPort;
        pairPort.second = oneRelaySession.usCalleeVRtcpPort;
        p_mtsTcp->GetPortPool()->RecycleUdpPort(pairPort);
        
        MTS_LOG_ERROR( "allocate relay port err,"
                      "surplus port nums: %u,"
                      "port pool item nums: %u,"
                      "port info: %s",
                      p_mtsTcp->GetPortPool()->GetUsablePortNums(),
                      p_mtsTcp->GetPortPool()->GetPortPoolCap(),
                      oneRelaySession.ToString().c_str() );

        m_stResp.iCode      = -1;
        m_stResp.sErrMsg    = "allocate relay udp port fail";
        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    }
    
    unsigned short usCmd        = UDP_CMD_ALLOC_PORT;
    oneRelaySession.usRelayCmd  = usCmd;
    bool bRet = SendToUdpSrv(oneRelaySession); 
    if (bRet == false)
    {
        MTS_LOG_ERROR("send open channel cmd: %d, to udp srv fail, session id: %s", 
                     usCmd, sSessionId.c_str());
        m_stResp.iCode      = -1;
        m_stResp.sErrMsg    = "open channel cmd to udp srv fail";
        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    }
    p_mtsTcp->GetPortPool()->m_mpClientSession.insert(std::make_pair(sSessionId, oneRelaySession));

    PolicyBusi::BuildRespDoc(oRoot);
    BuildSuccRet(oRoot, oneRelaySession);
    
    MTS_LOG_DEBUG("send open channel cmd: %d to udp srv succ, session id: %s", 
                  usCmd, sSessionId.c_str());
    return true;
}


void OpenChannel::BuildSuccRet(rapidjson::Document &oRoot, 
                               const RelayPortRecord& record)
{
    rapidjson::Document portObj;
    portObj.SetObject();
    rapidjson::Document::AllocatorType &port_allo = portObj.GetAllocator();
    portObj.AddMember("relay_caller_audio_rtp_port", record.usCallerARtpPort, port_allo);
    portObj.AddMember("relay_caller_audio_rtcp_port", record.usCallerARtcpPort, port_allo);
    portObj.AddMember("relay_caller_video_rtp_port", record.usCallerVRtpPort, port_allo);
    portObj.AddMember("relay_caller_video_rtcp_port", record.usCallerVRtcpPort, port_allo);
    portObj.AddMember("relay_callee_audo_io_rtp_port", record.usCalleeARtpPort, port_allo);
    portObj.AddMember("relay_callee_audo_io_rtcp_port", record.usCalleeARtcpPort, port_allo);
    portObj.AddMember("relay_callee_video_io_rtp_port", record.usCalleeVRtpPort, port_allo);
    portObj.AddMember("relay_callee_video_io_rtcp_port", record.usCalleeVRtcpPort, port_allo);

    rapidjson::Document resultObj;
    resultObj.SetObject();
    rapidjson::Document::AllocatorType &result_allo = resultObj.GetAllocator();
    resultObj.AddMember("port", portObj, result_allo);

    rapidjson::Document::AllocatorType &root_allo = oRoot.GetAllocator();
    oRoot.AddMember("result", resultObj, root_allo);

    return ;
}

//// 
ReleaseChannel::ReleaseChannel(const std::string& sMethod , MtsTcp* pMts )
    :PolicyBusi(sMethod, pMts)
{

}

ReleaseChannel::~ReleaseChannel()
{

}

bool ReleaseChannel::ProcessBusi(const rapidjson::Document &inRoot, rapidjson::Document &oRoot)
{
    ParseCommParams(inRoot);
    if ( !m_hasParams )
    {
        MTS_LOG_ERROR("has not params in ReleaseChannel req json");
        m_stResp.iCode = -1;
        m_stResp.sErrMsg = "req params not include";

        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    }

    const rapidjson::Value& paramVal = inRoot["params"];
    std::string sSessionId;

    do {
        if ( paramVal.HasMember("session_id") && paramVal["session_id"].IsString() )
        {
            sSessionId = paramVal["session_id"].GetString();
            MTS_LOG_DEBUG("method: %s, session id: %s", GetMethod().c_str(), sSessionId.c_str());
            break;
        }

        MTS_LOG_ERROR("not has session_id field on method: %s", GetMethod().c_str());
        
        m_stResp.iCode = -1;
        m_stResp.sErrMsg = "has not session_id in params obj";

        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    } while(0);


    auto itClient = p_mtsTcp->GetPortPool()->m_mpClientSession.find( sSessionId );
    if ( itClient == p_mtsTcp->GetPortPool()->m_mpClientSession.end() )
    {
        MTS_LOG_ERROR("session has not exist, session id: %s", sSessionId.c_str());

        m_stResp.iCode      = -1;
        m_stResp.sErrMsg    = "session has not exist";
        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    }
    RelayPortRecord &oneRelaySession = itClient->second;
    std::pair<unsigned short, unsigned short> pairRtpRtcp; 

    pairRtpRtcp.first = oneRelaySession.usCallerARtpPort;
    pairRtpRtcp.second = oneRelaySession.usCallerARtcpPort;
    p_mtsTcp->GetPortPool()->RecycleUdpPort( pairRtpRtcp );
    
    pairRtpRtcp.first = oneRelaySession.usCallerVRtpPort;
    pairRtpRtcp.second = oneRelaySession.usCallerVRtcpPort;
    p_mtsTcp->GetPortPool()->RecycleUdpPort( pairRtpRtcp );
    
    pairRtpRtcp.first = oneRelaySession.usCalleeARtpPort;
    pairRtpRtcp.second = oneRelaySession.usCalleeARtcpPort;
    p_mtsTcp->GetPortPool()->RecycleUdpPort( pairRtpRtcp );

    pairRtpRtcp.first = oneRelaySession.usCalleeVRtpPort;
    pairRtpRtcp.second = oneRelaySession.usCalleeVRtcpPort;
    p_mtsTcp->GetPortPool()->RecycleUdpPort( pairRtpRtcp );

    MTS_LOG_DEBUG("release relay port: callerARtp => %d,"
                  "callerARtcp => %d,"
                  "callerVRtp => %d,"
                  "callerVRtcp => %d,"
                  "calleeARtp => %d,"
                  "calleeARtcp => %d,"
                  "calleeVRtp => %d,"
                  "calleeVRtcp => %d, session id: %s",
                  oneRelaySession.usCallerARtpPort,
                  oneRelaySession.usCallerARtcpPort, 
                  oneRelaySession.usCallerVRtpPort,
                  oneRelaySession.usCallerVRtcpPort,
                  oneRelaySession.usCalleeARtpPort,
                  oneRelaySession.usCalleeARtcpPort,
                  oneRelaySession.usCalleeVRtpPort,
                  oneRelaySession.usCalleeVRtcpPort,
                  sSessionId.c_str());

    unsigned short usCmd        = UDP_CMD_RELEASE_PORT;
    oneRelaySession.usRelayCmd  = usCmd;

    bool bRet = SendToUdpSrv(oneRelaySession); 
    if (bRet == false)
    {
        MTS_LOG_ERROR("send open channel cmd: %d, to udp srv fail, session id: %s", 
                     usCmd, sSessionId.c_str());
        m_stResp.iCode      = -1;
        m_stResp.sErrMsg    = "release relay port cmd to udp srv fail";
        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    }

    p_mtsTcp->GetPortPool()->m_mpClientSession.erase( itClient );
    MTS_LOG_DEBUG( "release relay port cmd:[%d] send to udp srv succ, session id: %s",
                  usCmd, sSessionId.c_str() );
     this->BuildRespDoc(oRoot);
    return true;
}

bool ReleaseChannel::BuildRespDoc(rapidjson::Document &oRoot)
{
    return PolicyBusi::BuildRespDoc(oRoot);
}

bool ReleaseChannel::BuildRespParams()
{
    return PolicyBusi::BuildRespParams();
}
