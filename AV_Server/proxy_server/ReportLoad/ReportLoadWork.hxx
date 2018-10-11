#if !defined(RESIP_LOADREPORT_WORK_HXX)
#define RESIP_LOADREPORT_WORK_HXX

#include "ReportLoadContent.hxx"

#include <event2/event.h>
#include <event2/buffer.h>
#include <event.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "CBuffer.hpp"
#include "t_proto.hxx"
#include "t_socket.hxx"

#include "rapidjson/writer.h"                                                                                           
#include "rapidjson/stringbuffer.h"                                                                                     
#include "rapidjson/document.h"                                                                                         
#include "rapidjson/error/en.h"                                                                                         
#include "rapidjson/prettywriter.h" 

#include <map>
#include <string>
#include <memory>
#include <sstream>

using namespace rapidjson;


namespace repro
{

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

//--------------------------------------------
class ConnBase
{
 public:
  ConnBase(int iFd);
  virtual ~ConnBase();
  int GetFd() { return m_iFd; }
  //
  virtual bool AddEvent(struct event_base *pEvBase, int iEvent, void *arg) = 0;
  virtual bool DelEvent(struct event_base *pEvBase, int iEvent, void *arg) = 0;
  void DelWEvent();
  void DelREvent();
  //
 protected:
  int m_iFd;
  //
  struct event  m_stREvent;
  struct event  m_stWEvent;
};

//--------------------------------------------------------
class RL_Work;
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

  void SetLoadReport(RL_Work* pLoadReport)
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
  RL_Work*      m_pLoadReport;
};



//---------------------------------------------------------------------//
class RL_Work
{
 public:
  RL_Work(ReportLoadContent* prlCont, int iTms, 
          const std::string& sIp, unsigned int uiPort );
  virtual ~RL_Work();

  bool onStart();
  bool Process();
  //----------------------------------------------------//
  bool LoadAndSend();
  bool SendLoad(const std::string& sIp, 
                unsigned short usPort,
                rapidjson::Document& iRoot);
  bool BuildLoadPack(rapidjson::Document& oRoot);

  struct event_base* GetEventBase( )
  {
      return m_pEventBase;
  }

  bool AutoSend(const std::string& sIp, unsigned short usPort,
                rapidjson::Document& iRoot);

  bool SendLoad(const tagConnFd& conFd, rapidjson::Document& iRoot);
  
  std::shared_ptr<ConnAttr> CreateConnAttr(int iFd, unsigned int uiSeq);

  bool AddWriteEvent(std::shared_ptr<ConnAttr> pConnAttr);
  bool AddReadEvent(std::shared_ptr<ConnAttr> pConnAttr);

  bool EncodeSendData(std::shared_ptr<ConnAttr> pConnAttr, 
                      rapidjson::Document& iRoot);
  void DestroyConn(ConnAttr* pConnAttr);
 private:
  ReportLoadContent*                    m_pRLContent;
  int                                   m_iTms;
  std::string                           m_sHost;
  unsigned int                          m_uiPort;

  std::map<std::string, tagConnFd>              m_mpClientConn;
  std::map<int, std::shared_ptr<Sock> >         m_mpClientSock;
  std::map<int, std::shared_ptr<ConnAttr> >     m_mpConnAttr;

  struct event                          *m_TimerEvent;
  struct timeval                        *m_timeout;
  struct event_base                     *m_pEventBase;
};

}

#endif
