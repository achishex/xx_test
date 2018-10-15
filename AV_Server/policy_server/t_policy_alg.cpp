#include "t_policy_alg.h"
#include <limits.h>

LeastConnNumsAlg::LeastConnNumsAlg()
{
    m_iNodeLoadReportTm = ::atoi(ConfigXml::Instance()->getValue(
            "PolicyServer","ReportTm").c_str());
    if (m_iNodeLoadReportTm == 0)
    {
        m_iNodeLoadReportTm = 3*60;
    }

}

bool LeastConnNumsAlg::GetOptimServerNode(NodeLoad& node, int iNodeType)
{
    NodeLoadCollection::NodeLoadTypeMpPtr  optimNodeList;
    optimNodeList = NodeLoadCollection::Instance()->GetNodeInfo(iNodeType);
    if (!optimNodeList)
    {
        PL_LOG_ERROR("not get any node load info for node type: %d", iNodeType);
        return false;
    }
   
    int iLeastConnNums = INT_MAX;
    NodeLoad* pOptimNode = NULL;

    int iNowTm = ::time(NULL); 
    PL_LOG_DEBUG("cur tms: %d", iNowTm);
    for (const auto& load: *optimNodeList)
    {
        PL_LOG_DEBUG("cur tms:%d, node update tm:%d, cnf default expire tm:%d, node load:%d, cur load:%d", 
                     iNowTm,load.second->uiUpdateTm, m_iNodeLoadReportTm, load.second->m_iNodeConnNums,
                     iLeastConnNums);
        if ((load.second->m_iNodeConnNums  < iLeastConnNums) 
            && (iNowTm - load.second->uiUpdateTm) < m_iNodeLoadReportTm)
        {
            pOptimNode = (load.second).get();
            iLeastConnNums = load.second->m_iNodeConnNums;
        }
    }

    if (!pOptimNode)
    {
        PL_LOG_ERROR("not find node, its load less than: %d, nodetype: %d", iLeastConnNums, iNodeType);
        return false; 
    }

    std::swap(node, *pOptimNode);
    return true;
}
