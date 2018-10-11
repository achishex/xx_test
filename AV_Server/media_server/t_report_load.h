/**
 * @file: t_report_load.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-08-06
 */

#ifndef _T_REPORT_LOAD_H_
#define _T_REPORT_LOAD_H_

#include "rapidjson/writer.h"                                                                                           
#include "rapidjson/stringbuffer.h"                                                                                     
#include "rapidjson/document.h"                                                                                         
#include "rapidjson/error/en.h"                                                                                         
#include "rapidjson/prettywriter.h" 

#include <map>
#include <string>
#include "t_socket.hpp"
#include "mts_server.h"
#include "t_proto.hpp"
#include "t_eventbase.hpp"

class LoadReport;

struct tagConnFd
{
    int iFd;
    unsigned int uiSeq;

    tagConnFd(): iFd(-1), uiSeq(0)
    {  }

    tagConnFd(const tagConnFd& item)
    {
        iFd     = item.iFd;
        uiSeq   = item.uiSeq;
    }
    tagConnFd& operator = ( const tagConnFd& item )
    {
        if (this != &item)
        {
            iFd     = item.iFd;
            uiSeq   = item.uiSeq;
        }
        return *this;
    }
};

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
      m_sPerrNode = sNode;
  }

  std::string GetPeerNode() 
  {
      return m_sPerrNode;
  }

  void SetLoadReport(LoadReport* pLoadReport)
  {
      m_pLoadReport =  pLoadReport;
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
  std::string   m_sPerrNode;
  CBuffer*      m_pRecvBuff;   //接收缓冲区
  CBuffer*      m_pSendBuff;   //发送缓冲区
  LoadReport*   m_pLoadReport;
};

//----------------------------------------------------------------------//
class LoadReport
{
 public:
  LoadReport( MtsTcp* pMtsTcp );
  virtual ~LoadReport();

  bool Send(const std::string& sIp, unsigned short usPort,
            rapidjson::Document& iRoot);

  struct event_base* GetEventBase( )
  {
      if (m_pLocalTcpSrv)
      {
          return m_pLocalTcpSrv->GetEventBase( );
      }
      return NULL;
  }

  void DestroyConn(ConnAttr* pConnAttr);

 private:
  bool AutoSend(const std::string& sIp, 
                unsigned short usPort,
                rapidjson::Document& iRoot);

  bool Send(const tagConnFd& conFd, rapidjson::Document& iRoot);

  std::shared_ptr<ConnAttr> CreateConnAttr(int iFd, 
                                           unsigned int uiSeq);


  bool AddWriteEvent(std::shared_ptr<ConnAttr> pConnAttr);
  bool AddReadEvent(std::shared_ptr<ConnAttr> pConnAttr);

  bool EncodeSendData(std::shared_ptr<ConnAttr> pConnAttr, 
                      rapidjson::Document& iRoot);
 private:
  std::map<std::string, tagConnFd>              m_mpClientConn;
  std::map<int, std::shared_ptr<Sock> >         m_mpClientSock;
  std::map<int, std::shared_ptr<ConnAttr> >     m_mpConnAttr;
  MtsTcp*   m_pLocalTcpSrv;
};

#endif
