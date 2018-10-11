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
#include "mts_server.h"
#include "t_busi_module.h"
/**
 * @brief: 
 */
class OpenChannel: public PolicyBusi
{
 public:
  OpenChannel(const std::string& sMethod, MtsTcp* pMts);
  virtual ~OpenChannel();


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

  void BuildSuccRet(rapidjson::Document &oRoot, 
                   const RelayPortRecord& record);
};


/**
 * @brief: 
 */
//TODO:
class ReleaseChannel: public PolicyBusi
{
 public:
  ReleaseChannel( const std::string& sMethod, MtsTcp* pMts );
  virtual ~ReleaseChannel();

  virtual bool ProcessBusi(const rapidjson::Document &inRoot, rapidjson::Document &oRoot); 

 protected:
  virtual bool BuildRespDoc(rapidjson::Document &oRoot);
  virtual bool BuildRespParams();

 private:
  std::string m_sIp;
  int m_iPort;
};

#endif
