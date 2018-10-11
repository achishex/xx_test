#ifndef _NODE_LOAD_H_
#define _NODE_LOAD_H_

#include "LibSingleton.h"
#include "redis_pool.h"
#include <memory>
#include <mutex>
#include <map>

#define Policy_Node_Name    "Policy_Server"
#define Media_Node_Name     "Media_Server"
#define Proxy_Node_Name     "Proxy_Server"

//field name in db for nodeload
#define Load_NodeType_Name      "NodeType"
#define Load_NodeConnNum_Name   "NodeConnNum"
#define Load_Update_Name        "NodeUpdateTm"
#define Load_NodeIp_Name        "NodeIP"
#define Load_NodePort_Name      "NodePort"

/// * is ip:port
#define Node_Load_Key_Pre           "1:2"
#define Node_Load_Key_Pattern       "1:2:*"

enum NODE_TYPE_DEF
{
    EM_NODE_NONE    = 0,
    EM_POLICY_NODE  = 1,
    EM_MEDIA_NODE   = 2,
    EM_PROXY_NODE   = 3,
};

//
struct NodeLoad
{
  NodeLoad();
  virtual ~NodeLoad();
  
  int m_iNodeType;
  int m_iNodeConnNums;
  unsigned int uiUpdateTm;
  std::string m_sNodeIp;
  int m_iPort;
};

class NodeLoadCollection: public CSingleton<NodeLoadCollection>
{
 public:
    typedef std::map<std::string, std::shared_ptr<NodeLoad>>  NodeLoadTypeMp;
    typedef std::shared_ptr<NodeLoadTypeMp>  NodeLoadTypeMpPtr;

    NodeLoadCollection();
    virtual ~NodeLoadCollection();

    bool LoadAllNodeLoad();
    bool FlushDB();

    bool FlushOneLoadDB(const NodeLoad& nodeLoad, const std::string& sKey);

    std::shared_ptr<NodeLoad> GetOptimalNode();

    void UpdateLoadNode(NodeLoad& nodeLoad);
    void SetRedisHandle(std::shared_ptr<FS_RedisInterface> pRedisHandle);
    
    NodeLoadTypeMpPtr GetNodeInfo(int iNodeTye, const int iExpireTm = 0);

    int GetTypeNodeSize(int iNodeType);
    
    int GetNodeType(const std::string& sCmdNo)
    {
        int iRet = EM_NODE_NONE;
        auto it = m_mpCmdNoNodeType.find(sCmdNo);
        if (it == m_mpCmdNoNodeType.end())
        {
            return iRet;
        }

        iRet = it->second;
        return iRet;
    }

 private:
    std::shared_ptr<FS_RedisInterface> m_pRedisHandle;
 private:
    //key => ip:port
    NodeLoadTypeMp m_mpPLNodeLoad;
    std::mutex m_mtxPLNodeLoad;

    //key => ip:port
    NodeLoadTypeMp m_mpProxyNodeLoad;
    std::mutex m_mtxProxyNodeLoad;

    //key => ip:port
    NodeLoadTypeMp m_mpMediaNodeLoad;
    std::mutex m_mtxMediaNodeLoad;

    //in db,key format => 1:2:ip:port  

 private:
    std::map<std::string, int> m_mpCmdNoNodeType;
    int m_iNodeLoadExpireTm; //time when node load exist in local mem 
                             //must greate than client node report interval.

};

#endif
