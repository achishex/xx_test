#include <iostream>
#include "t_busi_module.h"
#include "mts_server.h"

bool PolicyBusi::ParseCommParams(const rapidjson::Document &inRoot)
{
    if (inRoot.HasParseError())
    {
        MTS_LOG_ERROR("input json has parse error");
        return false;
    }

    //
    if (inRoot.HasMember("method") && inRoot["method"].IsString())
    {
        m_stReq.m_sMethod = inRoot["method"].GetString();
    }

    if (inRoot.HasMember("timestamp") && inRoot["timestamp"].IsInt())
    {
        m_stReq.m_iTimeStamp = inRoot["timestamp"].GetInt();
    }

    if (inRoot.HasMember("req_id") && inRoot["req_id"].IsInt())
    {
        m_stReq.m_iReqId = inRoot["req_id"].GetInt();
    }

    if (inRoot.HasMember("traceId") && inRoot["traceId"].IsString())
    {
        m_stReq.m_sTraceId = inRoot["traceId"].GetString();
    }

    if (inRoot.HasMember("spanId") && inRoot["spanId"].IsString())
    {
        m_stReq.m_sSpandId = inRoot["spanId"].GetString();
    }

    m_hasParams = false;
    if (inRoot.HasMember("params") && inRoot["params"].IsObject())
    {
        m_hasParams = true;
    }
    return  PolicyBusi::BuildRespParams();   
}


bool PolicyBusi::BuildRespParams()
{
    m_stResp.m_sMethod = m_stReq.m_sMethod;
    m_stResp.m_iReqId = m_stReq.m_iReqId;
    m_stResp.m_sTraceId = m_stReq.m_sTraceId;
    m_stResp.m_sSpandId = m_stReq.m_sSpandId;
    m_stResp.m_iTimeStamp = time(NULL);
    m_stResp.iCode = 0;
    m_stResp.sErrMsg = std::string("succ");
    return true;
}

bool PolicyBusi::BuildRespDoc(rapidjson::Document &oRoot)
{
    oRoot.SetObject();
    rapidjson::Document::AllocatorType& allocator = 
        oRoot.GetAllocator();

    rapidjson::Value author;
    author.SetString(m_stResp.m_sMethod.c_str(), m_stResp.m_sMethod.size(), allocator);
    oRoot.AddMember("method", author, allocator);

    oRoot.AddMember("req_id", m_stResp.m_iReqId, allocator);

    author.SetString(m_stResp.m_sTraceId.c_str(), m_stResp.m_sTraceId.size(), allocator);
    oRoot.AddMember("traceId", author, allocator);

    author.SetString(m_stResp.m_sSpandId.c_str(), m_stResp.m_sSpandId.size(), allocator);
    oRoot.AddMember("spanId", author, allocator);

    oRoot.AddMember("code", m_stResp.iCode, allocator);

    author.SetString(m_stResp.sErrMsg.c_str(), m_stResp.sErrMsg.size(), allocator);
    oRoot.AddMember("msg", author, allocator);

    oRoot.AddMember("timestamp", m_stResp.m_iTimeStamp, allocator);
    
    return true;
}

bool PolicyBusi::SendToUdpSrv(const RelayPortRecord& relayRecord)
{
    return p_mtsTcp->SendToUdpSrvCmd((void*)&relayRecord, sizeof(relayRecord));
}
