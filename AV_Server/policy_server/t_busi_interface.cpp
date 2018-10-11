#include <iostream>
#include "t_busi_interface.h"

//TODO:
#ifndef  _debug_register 

#else
    #define debug_sip_ip    "10.101.70.53"
    #define debug_sip_port  5060
#endif


#define GET_NODE_TYPE_FUNC     NodeLoadCollection::Instance()->GetNodeType(GetMethod())



RegisterInterface::RegisterInterface(const std::string& sMethod)
    :PolicyBusi(sMethod)
{
}

RegisterInterface::~RegisterInterface()
{
}

bool RegisterInterface::ProcessBusi(const rapidjson::Document &inRoot,
                                    rapidjson::Document &oRoot)
{
    ParseCommParams(inRoot);
    if (!m_hasParams)
    {
        PL_LOG_ERROR("has not params in req json");
        m_stResp.iCode = -1;
        m_stResp.sErrMsg = "req params not include";
        
        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    }
    
    const rapidjson::Value& paramVal = inRoot["params"];
    m_hasParams = false;

    std::string sIP;
    int conn_nums = 0;
    int iPort = 0;

    do {
        if ( paramVal.HasMember("ip") && paramVal["ip"].IsString() )
        {
            sIP = paramVal["ip"].GetString();
            PL_LOG_DEBUG("method: %s, registe node ip: %s", GetMethod().c_str(), sIP.c_str());

            if (paramVal.HasMember("port") && paramVal["port"].IsInt())
            {
                iPort = paramVal["port"].GetInt();
                if (iPort > 0)
                {
                    if (paramVal.HasMember("load") && paramVal["load"].IsObject())
                    {
                        const rapidjson::Value& loadObj = paramVal["load"];

                        if (loadObj.HasMember("conn_nums") && loadObj["conn_nums"].IsInt())
                        {
                            conn_nums = loadObj["conn_nums"].GetInt();
                            PL_LOG_DEBUG("method: %s, ip: %s registe load_conn_nums: %d", GetMethod().c_str(), sIP.c_str(), conn_nums);
                            break;
                        }
                    }
                    PL_LOG_ERROR("method: %s, registe not take load info, ip: %s", GetMethod().c_str(), sIP.c_str());
                }
                PL_LOG_ERROR("registe port invaild,method: %s", GetMethod().c_str());
            }
            PL_LOG_ERROR("registe not take port, method: %s", GetMethod().c_str());
        }

        PL_LOG_ERROR("registe not take ip, method: %s", GetMethod().c_str());
        
        m_stResp.iCode = -1;
        m_stResp.sErrMsg = "registe not take ip or load info";

        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    } while(0);

    NodeLoad loadItem;

    loadItem.m_iNodeType        = GET_NODE_TYPE_FUNC;
    loadItem.m_iNodeConnNums    = conn_nums;
    loadItem.uiUpdateTm         = time(NULL);
    loadItem.m_sNodeIp          = sIP;
    loadItem.m_iPort            = iPort;

    NodeLoadCollection::Instance()->UpdateLoadNode(loadItem);
    PolicyBusi::BuildRespDoc(oRoot);
    
    PL_LOG_INFO("recv registe succ, peer ip: %s, port: %d, method: %s", 
                sIP.c_str(), iPort, GetMethod().c_str());
    return true;
}



/////////////////////////////////////////////////////////////////
ReportInterface::ReportInterface(const std::string& sMethod)
    :PolicyBusi(sMethod)
{

}

ReportInterface::~ReportInterface()
{
}

bool ReportInterface::ProcessBusi(const rapidjson::Document &inRoot, 
                            rapidjson::Document &oRoot)
{
    return true;
}



//////////////////////////////////////////////////////////////
NodeAllocate::NodeAllocate(const std::string& sMethod)
    :PolicyBusi(sMethod), m_sIp(""), m_iPort(0)
{
    m_algFactory = std::make_shared<LeastConnNumAlgFactory>(LeastConnNumAlgFactory());
}

NodeAllocate::~NodeAllocate()
{

}

bool NodeAllocate::ProcessBusi(const rapidjson::Document &inRoot, 
                                  rapidjson::Document &oRoot)
{
    if (!ParseCommParams(inRoot))
    {
        PL_LOG_ERROR("parse json fail, method: %s", GetMethod().c_str());
        m_stResp.iCode = -1;
        m_stResp.sErrMsg = "parse json fail";

        PolicyBusi::BuildRespDoc(oRoot);
        return false;
    }

    auto alg = m_algFactory->GetPolicyAlg(GET_NODE_TYPE_FUNC);

    NodeLoad oneNodeLoad;
    int errNums = 0;

RE_GET_OPTM_NODE:

    bool bRet = alg->GetOptimServerNode(oneNodeLoad, GET_NODE_TYPE_FUNC); 
    
    if (bRet == false)
    {
        PL_LOG_ERROR("get least conn num from local fail,method: %s", GetMethod().c_str());
        if (errNums >= 2)
        {
            PL_LOG_ERROR("logic err, exist loop for get optimic node,method: %s,err_times: %d", 
                         GetMethod().c_str(), errNums);

            m_stResp.iCode = -1;
            m_stResp.sErrMsg = "get node fail";
            PolicyBusi::BuildRespDoc(oRoot);
            return false;

        }

        int iNodeSize = NodeLoadCollection::Instance()->GetTypeNodeSize(GET_NODE_TYPE_FUNC);
        PL_LOG_DEBUG("get node size: %d for method: %s",iNodeSize, GetMethod().c_str());

        if (iNodeSize< 0)
        {
            PL_LOG_ERROR("get node from local mem fail, method: %s", GetMethod().c_str());

            m_stResp.iCode = -1;
            m_stResp.sErrMsg = "get node from local fail";
            PolicyBusi::BuildRespDoc(oRoot);
            return false;
        }

        if (iNodeSize > 0)
        {
            PL_LOG_ERROR("node exist local, but above get node fail, method: %s",
                         GetMethod().c_str());

            m_stResp.iCode = -1;
            m_stResp.sErrMsg = "node exit in local, but get fail";
            PolicyBusi::BuildRespDoc(oRoot);
            return false;
        }

        PL_LOG_INFO("type: %d node not any exist in local, so fetch from db, "
                    "method: %s, err_times: %d",
                    GET_NODE_TYPE_FUNC, GetMethod().c_str(), errNums);

        NodeLoadCollection::Instance()->LoadAllNodeLoad();
        
        errNums++;
//TODO:
#ifndef _debug_register 
        goto RE_GET_OPTM_NODE;
#else
        m_sIp = ConfigXml::Instance()->getValue("PolicyServer","ProxyIpDebug");
        m_iPort = ::atoi( ConfigXml::Instance()->getValue("PolicyServer","ProxyPortDebug").c_str() );
#endif

    }

//TODO:
#ifndef _debug_register 
    std::swap(m_sIp, oneNodeLoad.m_sNodeIp);
    m_iPort = oneNodeLoad.m_iPort;
#else
    m_sIp = ConfigXml::Instance()->getValue("PolicyServer","ProxyIpDebug");
    m_iPort = ::atoi( ConfigXml::Instance()->getValue("PolicyServer","ProxyPortDebug").c_str() );
#endif
    PL_LOG_DEBUG("get optimic sip, ip: %s, port: %d", m_sIp.c_str(), m_iPort);
    
    BuildRespParams();
    
    BuildRespDoc(oRoot);
    return true;
}

bool NodeAllocate::BuildRespParams()
{
    return  PolicyBusi::BuildRespParams();
}

bool NodeAllocate::BuildRespDoc(rapidjson::Document &oRoot)
{
    PolicyBusi::BuildRespDoc(oRoot);
    
    rapidjson::Value resultObj(rapidjson::kObjectType);

    rapidjson::Document::AllocatorType& allocator = oRoot.GetAllocator();
    resultObj.AddMember("port", m_iPort, allocator);

    rapidjson::Value author;
    author.SetString(m_sIp.c_str(), m_sIp.size(), allocator);
    resultObj.AddMember("ip", author, allocator);

    oRoot.AddMember("result", resultObj, allocator);

    return true;
}
