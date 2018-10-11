/**
 * @file: t_policy_alg.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x00001
 * @date: 2018-07-28
 */

#ifndef _POLICY_ALG_H_
#define _POLICY_ALG_H_

#include "t_node_load.h"
#include "policy_log.h"
#include "t_policy_alg.h"

#include <memory>
#include <map>


/**
 * @brief: base class of alg
 */
class PolicyAllocatNodeAlg
{

public:
    PolicyAllocatNodeAlg() {} 
    virtual ~PolicyAllocatNodeAlg() {}

    virtual bool GetOptimServerNode(NodeLoad& node, int iNodeType) = 0;
};

//--------------------//

/**
 * @brief: base class of alg factory
 */
class AlgFactory 
{
 public:
  AlgFactory() {}
  virtual ~AlgFactory() {}
 
  virtual std::shared_ptr<PolicyAllocatNodeAlg> GetPolicyAlg(int iNodeType) = 0;
};


/////////////////////////////////

/**
 * @brief: child class for alg, it is using least conn nums
 */
class LeastConnNumsAlg: public PolicyAllocatNodeAlg
{
 public:
  LeastConnNumsAlg(); 
  virtual ~LeastConnNumsAlg() {}
  bool GetOptimServerNode(NodeLoad& node, int iNodeType);

 private:
  unsigned int m_iNodeLoadReportTm;
};

//-------------------------//
/**
 * @brief: child class for alg factory
 */
class LeastConnNumAlgFactory: public AlgFactory
{
 public:
  LeastConnNumAlgFactory() {}
  ~LeastConnNumAlgFactory() {}
  virtual std::shared_ptr<PolicyAllocatNodeAlg> GetPolicyAlg(int iNodeType) 
  {
      return std::make_shared<LeastConnNumsAlg>(LeastConnNumsAlg());
  }
};

#endif
