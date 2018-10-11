/**
 * @file: mts_server.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x00001
 * @date: 2018-08-01
 */

#ifndef _AVS_MTS_SERVER_H_
#define _AVS_MTS_SERVER_H_

#include <map>
#include <string>
#include <memory>
#include <atomic>
#include <memory>
#include <thread>
#include <stdlib.h>

#include  <event2/event.h>
#include  <event2/buffer.h>
#include  <event.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rapidjson/writer.h"                                                                                           
#include "rapidjson/stringbuffer.h"                                                                                     
#include "rapidjson/document.h"                                                                                         
#include "rapidjson/error/en.h"                                                                                         
#include "rapidjson/prettywriter.h" 

class LoadReport;
class TimerReport;
class AcceptConn;
class ListenConn;
class PortPool;
class Sock;
class PolicyBusi;
class UdpClient;

using namespace rapidjson;

class MtsTcp
{
 public:
  MtsTcp(const std::string& sIp, unsigned int uiPort);
  virtual ~MtsTcp();
  bool Run();
  void SetLocalInternetIp(const std::string& sIp);
  void SetLocalInternetPort(unsigned int uiPort);

  bool DeleteAcceptConn(int iFd);
  struct event_base* GetEventBase()
  {
      return m_pEventBase;
  }

  void InitBusiModulePool();
  
  PortPool* GetPortPool();
  const std::string& GetUdpSrvIp() const 
  {
      return m_sUdpSrvIp;
  }
  unsigned short GetUdpSrvPort() 
  {
      return m_usUdpPort;
  }
  bool SendToUdpSrvCmd(const std::string& sIp, unsigned short usPort,
                       const void* pData, unsigned short iDataLen);
  bool LoadRport();
  bool Accept();

 public:
  std::map<std::string, std::shared_ptr<PolicyBusi>> m_mpBusiModule;

 private:
  bool Init();
  bool RegisterAcceptConn();
  bool AddNewConnReadEvent(AcceptConn* pAcceptConn);

  bool BuildLoadPack(rapidjson::Document& oRoot);

private:
  std::string                   m_sSrvIp;
  unsigned int                  m_uiSrvPort;
  
  std::string                   m_sSrvInternetIp;
  unsigned int                  m_uiSrvInternetPort;

  bool                          m_bInit;
  Sock*                         m_pListenSock;
  struct event_base*            m_pEventBase;
  
  ListenConn*                   m_pListenConn;
  PortPool*                     m_pPortPool;

  std::string                   m_sUdpSrvIp;
  unsigned short                m_usUdpPort;

  std::map<int, AcceptConn*>    m_mpClientConn;
  std::atomic<int>              m_iCountAccept;

  std::string                   m_sPolicyIp;
  unsigned short                m_usPolicyPort;
  TimerReport*                  m_pTimerRport;
  LoadReport*                   m_pLoadReport;
 
  std::unique_ptr<UdpClient>    m_pUdpClient;
};

#endif
