#ifndef _T_TCP_CLIENT_H_
#define _T_TCP_CLIENT_H_

#include "rapidjson/writer.h"                                                                                           
#include "rapidjson/stringbuffer.h"                                                                                     
#include "rapidjson/document.h"                                                                                         
#include "rapidjson/error/en.h"                                                                                         
#include "rapidjson/prettywriter.h" 

#include <map>
#include <string>
#include <memory>
#include "t_socket.hpp"
#include "t_proto.hpp"
#include "t_eventbase.hpp"
#include "t_conn_data.h"

class TcpClient;

class ConnAttr: public ConnBase
{
 public:
  ConnAttr(int iFd = -1, unsigned int iSeq = 0);
  virtual ~ConnAttr();

  void SetClientStatus(int iStatus)
  {
      m_iClientStatus = iStatus;
  }
  void SetSeq(int uiSeq)
  {
      m_iSeq = uiSeq;
  }

  virtual bool AddEvent(struct event_base *pEvBase, int iEvent, void *arg);
  virtual bool DelEvent(struct event_base *pEvBase, int iEvent, void *arg);

  bool EncodeData(rapidjson::Document& iRoot);

  void SetPeerNode(const std::string& sNode)
  {
      m_sPeerNode = sNode;
  }

  std::string GetPeerNode() 
  {
      return m_sPeerNode;
  }

  void SetTcpClient( TcpClient* pHandle )
  {
      m_pTcpClientHandle = pHandle;
  }

 private:
  static void ReadCallBack(int iFd, short sEvent, void *pData); 
  static void WriteCallBack(int iFd, short sEvent, void *pData);

  void IoWrite( int iFd );
  void IoRead( int iFd );

  bool DoBusiProc(rapidjson::Document& retDoc);
 private:
  BusiCodec*    m_pCodec;
  int           m_iSeq;
  int           m_iClientStatus; //0: init, 1: connecting, 2: conntected
  std::string   m_sPeerNode;
  CBuffer*      m_pRecvBuff;   //接收缓冲区
  CBuffer*      m_pSendBuff;   //发送缓冲区
  TcpClient*    m_pTcpClientHandle;
};

//------------------------------------------------------------------------//
class TcpClient 
{
 public:
  TcpClient( struct event_base* pEventBase );
  virtual ~TcpClient();

  bool Send(const std::string& sDstIp, unsigned short usDstPort,
            rapidjson::Document& iRoot);
  struct event_base* GetEventBase( )
  {
    return base;
  }

  void DestroyConn(ConnAttr* pConnAttr);

 protected:
  bool AutoSend(const std::string& sIp, 
                unsigned short usPort,
                rapidjson::Document& iRoot);

  bool Send(const tagConnFd& conFd, rapidjson::Document& iRoot);
  std::shared_ptr<ConnAttr> CreateConnAttr(int iFd, unsigned int uiSeq);
                                           
  bool AddWriteEvent(std::shared_ptr<ConnAttr> pConnAttr);
  bool AddReadEvent(std::shared_ptr<ConnAttr> pConnAttr);

  bool EncodeSendData(std::shared_ptr<ConnAttr> pConnAttr, 
                      rapidjson::Document& iRoot);

  std::map<std::string, tagConnFd>              m_mpClientConn;
  std::map<int, std::shared_ptr<Sock> >         m_mpClientSock;
  std::map<int, std::shared_ptr<ConnAttr> >     m_mpConnAttr;
  struct event_base *base;
};

#endif
