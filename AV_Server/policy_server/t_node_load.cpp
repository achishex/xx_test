#include <time.h>
#include "t_node_load.h"
#include "policy_log.h"
#include "proto_inner.h"

NodeLoad::NodeLoad(): m_iNodeType(EM_NODE_NONE), 
                      m_iNodeConnNums(0),
                      uiUpdateTm(time(NULL)),
                      m_iPort(0)
{ }

NodeLoad::~NodeLoad()
{ }


NodeLoadCollection::NodeLoadCollection()
{
    //not add mutex
    m_mpCmdNoNodeType[CMD_NO_REGISTER_SIP_REQ] =  EM_PROXY_NODE;
    m_mpCmdNoNodeType[CMD_NO_REPORT_SIP_REQ]   =  EM_PROXY_NODE; 
    m_mpCmdNoNodeType[CMD_NO_GET_SIP_REQ]      =  EM_PROXY_NODE;
    
    m_mpCmdNoNodeType[CMD_NO_REGISTER_MTS_REQ] =  EM_MEDIA_NODE;
    m_mpCmdNoNodeType[CMD_NO_REPORT_MTS_REQ]   =  EM_MEDIA_NODE;
    m_mpCmdNoNodeType[CMD_NO_GET_MTS_REQ]      =  EM_MEDIA_NODE;

    //init each expire tm in local mem 
    m_iNodeLoadExpireTm = ::atoi(ConfigXml::Instance()->getValue(
            "PolicyServer", "RedisExpireTm").c_str());
    if (m_iNodeLoadExpireTm == 0)
    {
        m_iNodeLoadExpireTm = 5*60;
    }
}

NodeLoadCollection::~NodeLoadCollection()
{
    {
        std::lock_guard<std::mutex> mtxLock(m_mtxPLNodeLoad);
        m_mpPLNodeLoad.clear();
    }

    {
        std::lock_guard<std::mutex> mtxLock(m_mtxProxyNodeLoad);
        m_mpProxyNodeLoad.clear();
    }

    {
        std::lock_guard<std::mutex> mtxLock(m_mtxMediaNodeLoad);
        m_mpMediaNodeLoad.clear();
    }
}

void NodeLoadCollection::UpdateLoadNode(NodeLoad& nodeLoad)
{
    if (nodeLoad.m_sNodeIp.empty())
    {
        return ;
    }
    
    std::mutex*  pMutex =  NULL;
    NodeLoadTypeMp* pMpNodeLoad = NULL;
    
    if (nodeLoad.m_iNodeType == EM_POLICY_NODE)
    {
        pMutex = &m_mtxPLNodeLoad; 
        pMpNodeLoad = &m_mpPLNodeLoad;
    } 
    else if (EM_MEDIA_NODE == nodeLoad.m_iNodeType)
    {

        pMutex = &m_mtxMediaNodeLoad; 
        pMpNodeLoad = &m_mpMediaNodeLoad;

    } 
    else if (EM_PROXY_NODE == nodeLoad.m_iNodeType)
    {
        pMutex = &m_mtxProxyNodeLoad; 
        pMpNodeLoad = &m_mpProxyNodeLoad;
    }
    else 
    {
        return ;
    }

    if (pMutex != NULL && pMpNodeLoad != NULL)
    {
        std::stringstream ss;
        
        std::lock_guard<std::mutex> lock(*pMutex);
        ss << nodeLoad.m_sNodeIp << ":" << nodeLoad.m_iPort;

        (*pMpNodeLoad)[ss.str().c_str()] =  std::make_shared<NodeLoad>(std::move(nodeLoad));
    }
}


void NodeLoadCollection::SetRedisHandle(std::shared_ptr<FS_RedisInterface> pRedisHandle)
{
    m_pRedisHandle = pRedisHandle;
}


bool NodeLoadCollection::LoadAllNodeLoad()
{
    if (!m_pRedisHandle)
    {
        PL_LOG_ERROR("redis handle is null");
        return false;
    }

    std::vector<std::string> vNodeLoad; 
    int iCurNums = 0;
    do {
        std::vector<std::string> vBatchNodeLoad;
        bool bRet = m_pRedisHandle->Scan(iCurNums, vBatchNodeLoad, Node_Load_Key_Pattern, 100);
        if (bRet == false)
        {
            PL_LOG_ERROR("scan node load key fail");
            break;
        }

        vNodeLoad.insert(vNodeLoad.end(), vBatchNodeLoad.begin(), vBatchNodeLoad.end());
    } while(iCurNums != 0); 

    for (const auto &nodeLoadKey: vNodeLoad)
    {
        std::map<std::string, std::string> mpNodeLoad;
        bool bRet = m_pRedisHandle->HGetAll(nodeLoadKey, mpNodeLoad);
        if (bRet == false)
        {
            PL_LOG_ERROR("hgetall fail for key: %s", nodeLoadKey.c_str());
            continue;
        }

        for (const auto &nodeLoadField: mpNodeLoad)
        {
            NodeLoad stNodeLoad;
            if (nodeLoadField.first == Load_NodeType_Name)
            {
                stNodeLoad.m_iNodeType = ::atoi(nodeLoadField.second.c_str());
            }
            else if (nodeLoadField.first == Load_NodeConnNum_Name)
            {
                stNodeLoad.m_iNodeConnNums = ::atoi(nodeLoadField.second.c_str());
            }
            else if (nodeLoadField.first == Load_Update_Name)
            {
                stNodeLoad.uiUpdateTm = ::atoi(nodeLoadField.second.c_str()); 
            }
            else if (nodeLoadField.first == Load_NodeIp_Name)
            {
                stNodeLoad.m_sNodeIp = nodeLoadField.second;
            }
            else if (nodeLoadField.first == Load_NodePort_Name)
            {
                stNodeLoad.m_iPort = ::atoi(nodeLoadField.second.c_str());
            }
            else 
            {
                continue;
            }
            NodeLoadCollection::Instance()->UpdateLoadNode(stNodeLoad);
        }
    }

    PL_LOG_INFO( "load node load from redis done, node load nums: %d ", vNodeLoad.size() );
    return true;
}

NodeLoadCollection::NodeLoadTypeMpPtr  
NodeLoadCollection::GetNodeInfo(int iNodeTye, const int iExpireTm)
{
    NodeLoadTypeMpPtr retNode;
    int nowTm = 0;
    if (iExpireTm > 0)
    {
        nowTm = ::time(NULL);
    }

    std::mutex*  pMutex =  NULL;
    NodeLoadTypeMp* pMpNodeLoad = NULL;

    if (iNodeTye == EM_POLICY_NODE)
    {
        pMutex      = &m_mtxPLNodeLoad; 
        pMpNodeLoad = &m_mpPLNodeLoad;
    } 
    else if (EM_MEDIA_NODE == iNodeTye)
    {
        pMutex      = &m_mtxMediaNodeLoad; 
        pMpNodeLoad = &m_mpMediaNodeLoad;

    } 
    else if (EM_PROXY_NODE == iNodeTye)
    {
        pMutex      = &m_mtxProxyNodeLoad; 
        pMpNodeLoad = &m_mpProxyNodeLoad;
    }
    else 
    {
        return retNode;
    }

    {
        std::lock_guard<std::mutex> lock(*pMutex);
        if ( pMpNodeLoad->empty() )
        {
            return retNode;
        }

        retNode = std::make_shared<NodeLoadTypeMp>(NodeLoadTypeMp());

        for (auto iterNode = pMpNodeLoad->begin(); iterNode!= pMpNodeLoad->end();)
        {
            if (nowTm <= 0)
            {
                (*retNode)[iterNode->first] = std::make_shared<NodeLoad>(*(iterNode->second));
                iterNode ++;
                continue;
            }

            //checkout expire
            int iDiff = nowTm - iterNode->second->uiUpdateTm;
            if (iDiff < iExpireTm)
            {
                (*retNode)[iterNode->first] = std::make_shared<NodeLoad>(*(iterNode->second));
                iterNode ++;
                continue;
            }
            pMpNodeLoad->erase(iterNode++);
        }
    }

    return (retNode);
}


int NodeLoadCollection::GetTypeNodeSize(int iNodeType)
{
    std::mutex*  pMutex =  NULL;
    NodeLoadTypeMp* pMpNodeLoad = NULL;
    int iRet = 0;

    if (iNodeType == EM_POLICY_NODE)
    {
        pMutex = &m_mtxPLNodeLoad; 
        pMpNodeLoad = &m_mpPLNodeLoad;
    } 
    else if (EM_MEDIA_NODE == iNodeType)
    {
        pMutex = &m_mtxMediaNodeLoad; 
        pMpNodeLoad = &m_mpMediaNodeLoad;

    } 
    else if (EM_PROXY_NODE == iNodeType)
    {
        pMutex = &m_mtxProxyNodeLoad; 
        pMpNodeLoad = &m_mpProxyNodeLoad;
    }
    else 
    {
        iRet = -1;
    }
   
    {
        std::lock_guard<std::mutex> lock(*pMutex);
        iRet = pMpNodeLoad->size();
    }
    return iRet;
}

bool NodeLoadCollection::FlushDB()
{
    NodeLoadTypeMpPtr pMTSNodeLoad      = GetNodeInfo(EM_MEDIA_NODE, m_iNodeLoadExpireTm);
    NodeLoadTypeMpPtr pPLNodeLoad       = GetNodeInfo(EM_POLICY_NODE, m_iNodeLoadExpireTm);
    NodeLoadTypeMpPtr pProxySipNodeLoad = GetNodeInfo(EM_PROXY_NODE, m_iNodeLoadExpireTm);

    if (pMTSNodeLoad != nullptr)
    {
        for (const auto &nodeLoad: *pMTSNodeLoad)
        {
            std::stringstream ss;
            ss << Node_Load_Key_Pre << ":" << nodeLoad.first;
            bool bRet = FlushOneLoadDB(*(nodeLoad.second), ss.str().c_str());
            if (bRet == false)
            {
                PL_LOG_ERROR("write mts node load to db fail, key: %s", ss.str().c_str());
                continue;
            }
        }
    }
    else 
    {
        PL_LOG_DEBUG("has not any load for mts");
    }

    if (pPLNodeLoad != nullptr)
    {
        for (const auto& nodeLoad: *pPLNodeLoad)
        {
            std::stringstream ss;
            ss << Node_Load_Key_Pre << ":" << nodeLoad.first;
            bool bRet = FlushOneLoadDB(*(nodeLoad.second), ss.str().c_str());
            if (bRet == false)
            {
                PL_LOG_ERROR("write pl node load to db fail, key: %s", ss.str().c_str());
                continue;
            }
        }
    }
    else
    {
        PL_LOG_DEBUG("has not any load for policy server");
    }

    if (pProxySipNodeLoad != nullptr)
    {
        for (const auto& nodeLoad: *pProxySipNodeLoad)
        {
            std::stringstream ss;
            ss << Node_Load_Key_Pre << ":" << nodeLoad.first;
            bool bRet = FlushOneLoadDB(*(nodeLoad.second), ss.str().c_str());
            if (bRet == false)
            {
                PL_LOG_ERROR("write proxy node load to db fail, key: %s", ss.str().c_str());
                continue;
            }
        }
    }
    else
    {
        PL_LOG_DEBUG("has not any load for sip server");
    }

    return true;
}

bool NodeLoadCollection::FlushOneLoadDB(const NodeLoad& nodeLoad, const std::string& sKey)
{

	

    if (sKey.empty() || !m_pRedisHandle)
    {
        return false;
    }

    std::map<std::string, std::string> mpFieldValue;
    
    mpFieldValue[Load_NodeType_Name]        = std::to_string(nodeLoad.m_iNodeType);
    mpFieldValue[Load_NodeConnNum_Name]     = std::to_string(nodeLoad.m_iNodeConnNums);
    mpFieldValue[Load_Update_Name]          = std::to_string(nodeLoad.uiUpdateTm);
    mpFieldValue[Load_NodeIp_Name]          = nodeLoad.m_sNodeIp;
    mpFieldValue[Load_NodePort_Name]        = std::to_string(nodeLoad.m_iPort);

    bool bRet = m_pRedisHandle->HMSet(sKey, mpFieldValue);
    if (bRet == false)
    {
        PL_LOG_ERROR("hmset cmd fail, key: %s", sKey.c_str());
        return false;
    }

    bRet = m_pRedisHandle->ExpireTm(sKey, m_iNodeLoadExpireTm);
    if (bRet == false)
    {
        PL_LOG_ERROR("expire cmd fail, key: %s, exire tm: %d s",
                     sKey.c_str(), m_iNodeLoadExpireTm);
        return false;
    }
    return true;
}
//
