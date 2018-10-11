/**
 * @file: t_busi_module.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x00001
 * @date: 2018-07-28
 */

#ifndef _POLICY_BUSI_MODULE_H_
#define _POLICY_BUSI_MODULE_H_


#include "rapidjson/writer.h"                                                                                       
#include "rapidjson/stringbuffer.h"                                                                                 
#include "rapidjson/document.h"                                                                                     
#include "rapidjson/error/en.h"                                                                                     
#include "rapidjson/prettywriter.h" 

#include <string>
#include <time.h>
#include <memory>

#include "policy_log.h"

class PolicyBusi
{
 public:
  struct ParamComm
  {
      ParamComm() :m_sMethod(""),
                   m_iReqId(0),
                   m_sTraceId(""),
                   m_sSpandId(""), 
                   m_iTimeStamp(time(NULL)),
                   iCode(0), sErrMsg("") {
                    //
      }




      std::string m_sMethod;
      int m_iReqId;
      std::string m_sTraceId;
      std::string m_sSpandId;
      int m_iTimeStamp;
      //response
      int iCode;
      std::string sErrMsg;
  };

 public:
  PolicyBusi(const std::string& sMethod = "")
      :m_sMethod(sMethod)
  { m_hasParams = false; }

  virtual ~PolicyBusi() 
  { }
    

  /**
   * @brief: ProcessBusi 
   *
   * @param inRoot: has parsed from string.
   * @param oRoot: empty json.
   *
   * @return 
   */
  virtual bool ProcessBusi(const rapidjson::Document &inRoot, rapidjson::Document &oRoot) = 0;
 
 protected:

  /**
   * @brief: ParseCommParams 
   *  解析请求msg的公共部分,并按默认方式填充response字段
   *
   * @param inRoot
   *
   * @return 
   */
  virtual bool ParseCommParams(const rapidjson::Document &inRoot);

  /**
   * @brief: BuildRespParams 
   * 填充response 的默认字段，默认值
   *
   * @return 
   */
  virtual bool BuildRespParams();


  /**
   * @brief: BuildRespDoc 
   * 构建response josn 基本数据格式
   *
   * @param oRoot
   *
   * @return 
   */
  virtual bool BuildRespDoc(rapidjson::Document &oRoot);

  const std::string& GetMethod() const 
  {
      return m_sMethod;
  }
 protected:
  std::string m_sMethod;
  ParamComm m_stReq;
  ParamComm m_stResp;

  //for some req has params field;
  bool m_hasParams;

};


#endif
