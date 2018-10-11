/**
 * @file: t_busi_interface.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-07-28
 */


#ifndef _BUSI_INTERFACE_H_
#define _BUSI_INTERFACE_H_

#include <iostream>
#include "t_busi_module.h"
#include "t_node_load.h"
#include "t_policy_alg.h"
/**
 * @brief: 
 */
class RegisterInterface: public PolicyBusi
{
 public:
  RegisterInterface(const std::string& sMethod = "");
  virtual ~RegisterInterface();


  /**
   * @brief: ProcessBusi 
   * 
   * @param inRoot
   * @param oRoot
   *
   * @return 
   */
  virtual bool ProcessBusi(const rapidjson::Document &inRoot,
                           rapidjson::Document &oRoot); 
 protected:

  virtual bool BuildRespDoc(rapidjson::Document &oRoot)
  {
      return PolicyBusi::BuildRespDoc(oRoot);
  }
  virtual bool BuildRespParams() 
  {
      return PolicyBusi::BuildRespParams();
  }
};


/**
 * @brief: 在使用上复用注册流程, 此处不实现其定义
 */
class ReportInterface: public PolicyBusi
{
 public:
  ReportInterface(const std::string& sMethod = "");
  virtual ~ReportInterface();

  virtual bool ProcessBusi(const rapidjson::Document &inRoot, rapidjson::Document &oRoot); 
 
 protected:
  virtual bool BuildRespDoc(rapidjson::Document &oRoot) { return true; }
  virtual bool BuildRespParams() { return true; }
};



/**
 * @brief: 
 */
class NodeAllocate: public PolicyBusi
{
 public:
  NodeAllocate(const std::string& sMethod = "");
  virtual ~NodeAllocate();

  virtual bool ProcessBusi(const rapidjson::Document &inRoot, rapidjson::Document &oRoot); 

 protected:
  virtual bool BuildRespDoc(rapidjson::Document &oRoot);
  virtual bool BuildRespParams();

 private:
  std::shared_ptr<AlgFactory> m_algFactory;
  std::string m_sIp;
  int m_iPort;
};

#endif
