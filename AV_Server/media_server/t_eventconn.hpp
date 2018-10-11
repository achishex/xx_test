/**
 * @file: t_eventconn.hpp
 * @brief: 
 *      client 已连接对象的实现，线程或者工作线程用该对象进行数据的收发，
 *      业务逻辑的处理等。
 *      目前是采用一个线程一个连接。后续可优化为: 业务用独立的线程来完成，
 *      处理完后再用接收数据的线程发送出去。
 * @author:  wusheng Hu
 * @version: v0x00001
 * @date: 2018-04-24
 */

#ifndef _T_EVENTCONN_HPP_
#define _T_EVENTCONN_HPP_

#include "CBuffer.hpp"
#include "t_eventbase.hpp"

#include "avs_mts_log.h"
#include "t_busi_module.h"

class BusiCodec;
class MtsTcp;

class AcceptConn: public ConnBase
{
 public:
  AcceptConn(int ifd, void *pData);
  virtual ~AcceptConn();
  virtual bool AddEvent(struct event_base *pEvBase, int iEvent, void *arg);
  virtual bool DelEvent(struct event_base *pEvBase, int iEvent, void *arg);

  std::shared_ptr<PolicyBusi> GetBusiModule( const std::string& sMethod );

 private:
  static void ReadCallBack(int iFd, short sEvent, void *pData); 
  static void WriteCallBack(int iFd, short sEvent, void *pData);

  bool DoRead();
  bool DoWrite();

  std::string GetKeyItemVal(const std::string& sKey, rapidjson::Document &root);
  void BuildErrResp(rapidjson::Document& oroot, 
                    rapidjson::Document& inroot, 
                    int iCode, const std::string& sErrMsg);

 private:
  MtsTcp*       m_pData;
  BusiCodec*    m_pCodec;

 private:
  CBuffer*      pRecvBuff;   //接收缓冲区
  CBuffer*      pSendBuff;   //发送缓冲区
};
#endif
