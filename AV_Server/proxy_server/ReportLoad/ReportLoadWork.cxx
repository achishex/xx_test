#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rutil/Logger.hxx"
#include "ReportLoadWork.hxx"
#include "protocol/proto_inner.h"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;
using namespace rapidjson;

#include <unistd.h>
#include <iostream>

ConnBase::ConnBase(int iFd):m_iFd(iFd) 
{ }

ConnBase::~ConnBase()
{ 
    event_del(&m_stREvent);
    event_del(&m_stWEvent);
    DebugLog( << "close conn fd: " <<  m_iFd);
    ::close(m_iFd);
    m_iFd = -1;
}

void ConnBase::DelWEvent()
{
    event_del(&m_stWEvent);
}
void ConnBase::DelREvent()
{
    event_del(&m_stREvent);
}

//------------------------------------------------------//
ConnAttr::ConnAttr(int iFd, unsigned int iSeq)
                  : ConnBase(iFd), m_iSeq(iSeq), 
                  m_iClientStatus(0), m_pLoadReport(NULL)
{
    m_pCodec    = new BusiCodec(1);
    m_pRecvBuff = new CBuffer();
    m_pSendBuff = new CBuffer();
}

ConnAttr::~ConnAttr()
{
    if(m_pCodec)
    {
        delete m_pCodec;
        m_pCodec = NULL;
    }
    if (m_pRecvBuff)
    {
        delete m_pRecvBuff;
        m_pRecvBuff = NULL;
    }
    if (m_pSendBuff)
    {
        delete m_pSendBuff;
        m_pSendBuff = NULL;
    }
    m_iClientStatus = 0;
}

bool ConnAttr::EncodeData(rapidjson::Document& iRoot)
{
    if ( RET_RECV_OK != m_pCodec->EnCode(m_pSendBuff, iRoot) )
    {
        ErrLog( << "encode send buf data fail");
        return false;
    } 
    return true;
}

bool ConnAttr::AddEvent(struct event_base *pEvBase, int iEvent, void *arg)
{
    RL_Work* pLoadReport = (RL_Work*)arg;

    if (pEvBase == NULL)
    {
        return false;
    }

    this->SetLoadReport(pLoadReport);

    if (EV_READ & iEvent)
    {
        event_assign(&m_stREvent, pEvBase, m_iFd, iEvent, ReadCallBack, this);
        event_add(&m_stREvent, NULL); 
    } 
    else 
    {
        event_assign(&m_stWEvent, pEvBase, m_iFd, iEvent, WriteCallBack, this);
        event_add(&m_stWEvent, NULL);
    }
    return true;
}

bool ConnAttr::DelEvent(struct event_base *pEvBase, int iEvent, void *arg)
{
    if (pEvBase == NULL)
    {
        return false;
    }
    return true;
}


void ConnAttr::ReadCallBack(int iFd, short sEvent, void *pData)
{
    ConnAttr* pConnAttr = (ConnAttr*)pData;
    pConnAttr->IoRead(iFd);
}

void ConnAttr::WriteCallBack(int iFd, short sEvent, void *pData)
{
    ConnAttr* pConnAttr = (ConnAttr*)pData;
    pConnAttr->IoWrite(iFd);
}

void ConnAttr::IoWrite(int iFd)
{
    if (m_pLoadReport == NULL)
    {
        ErrLog( << "load report handle is null");
        return ;
    }

    if (iFd <= 0)
    {
        ErrLog( << "write event fd is invalid, fd: " << iFd);
        return ;
    }

    if ( m_iClientStatus == 1 )
    {
        int iSockErr, iSockErrLen = sizeof(iSockErr);
        int iRet  = ::getsockopt( iFd, SOL_SOCKET, SO_ERROR, &iSockErr, (socklen_t *)&iSockErrLen );
        if ( iRet != 0 || iSockErr )
        {
            ErrLog( << "getsockopt failed (errno " 
                   << errno 
                   << ")!, async connect sock err: " 
                   << iSockErr);
            m_pLoadReport->DestroyConn(this);
            return ;
        }

        m_iClientStatus = 2;
        InfoLog( <<"client has connected, fd: " << iFd);
    }

    int iErrno = 0;
    int iNeedWtLen = m_pSendBuff->ReadableBytes();
    int iWritenLen = m_pSendBuff->WriteFD(iFd, iErrno);
    m_pSendBuff->Compact(8192);

    if (iWritenLen < 0)
    {
        if ( EAGAIN != iErrno && EINTR != iErrno )
        {
            ErrLog( << "write data err on fd: " << iFd << " err msg: " << strerror(errno));
            m_pLoadReport->DestroyConn(this);
            return ;
        }

        bool bRet = AddEvent( m_pLoadReport->GetEventBase(), EV_WRITE, m_pLoadReport );
        if (bRet == false)
        {
            ErrLog( << "add write event again fail");
        }
    }
    else if (iWritenLen >0)
    {
        if (iWritenLen == iNeedWtLen)
        {
            DebugLog( << "write data to peer node done, fd: " << iFd);
            return ;
        }
        else 
        {
            AddEvent( m_pLoadReport->GetEventBase(), EV_WRITE, m_pLoadReport );
        }
    }
    else
    {
        ErrLog( << "has no more data to send on fd: " << iFd);
    }
}

void ConnAttr::IoRead(int iFd)
{
    if (iFd <= 0)
    {
        return ;
    }

    if ( m_pLoadReport == NULL )
    {
        return ;
    }

    int iErrno = 0;
    m_pRecvBuff->Compact(8192);
    int iReadLen = m_pRecvBuff->ReadFD(iFd, iErrno);
    DebugLog( << "recv data from fd: " << iFd << ", data len: " << iReadLen);

    if (iReadLen > 0)
    {
        rapidjson::Document retDoc;
        CODERET retCode =  m_pCodec->DeCode( m_pRecvBuff, retDoc );
        if (retCode == RET_RECV_OK)
        {
            DoBusiProc(retDoc);
        }
        else if (retCode == RET_RECV_ERR)
        {
            ErrLog( << "decode recv data err, on fd: " << iFd );
            m_pLoadReport->DestroyConn(this);
        }
        else
        {
            return ;
        }
    }
    else if (iReadLen == 0)
    {
        InfoLog( << "peer node active close this conn, err msg: " << strerror(iErrno));
        m_pLoadReport->DestroyConn(this);
        return ;
    }
    else 
    {
        if (EAGAIN != iErrno && EINTR != iErrno)
        {
            ErrLog( << "recv data has err, err msg: " << strerror(iErrno));
            m_pLoadReport->DestroyConn(this);
            return ;
        }
    }
}

bool ConnAttr::DoBusiProc(rapidjson::Document& retDoc)
{
    if (retDoc.HasParseError())
    {
        ErrLog( << "parse response json fail");
        return false;
    }

    int iSeq = 0;
    if (retDoc.HasMember("req_id") && retDoc["req_id"].IsInt())
    {
        iSeq = retDoc["req_id"].GetInt();
    }

    if (m_iSeq != iSeq)
    {
        ErrLog( << "recv response seq: " << iSeq << " !=  req seq: " << m_iSeq);
        return false;
    }

    int iCode = 0;
    if ( retDoc.HasMember("code") && retDoc["code"].IsInt() )
    {
        iCode = retDoc["code"].GetInt();
        DebugLog( << "response code: " << iCode);
    }
    std::string sMsg = retDoc["msg"].GetString();

    if ( retDoc.HasMember("msg") && retDoc["msg"].IsString() )
    {
        if (iCode != 0)
        {
            ErrLog( << "response err msg: " << sMsg);
            return false;
        }
        else
        {
            DebugLog( <<"reponse msg: " << sMsg);
            return true;
        }
    }
    return true;
}

//--------------------------------------------------------------------//
RL_Work::RL_Work(ReportLoadContent* prlCont, int iTms,
                 const std::string& sIp, unsigned int uiPort)
    : m_pRLContent(prlCont), m_iTms(iTms), m_sHost(sIp), m_uiPort(uiPort),
    m_TimerEvent(NULL), m_timeout(NULL), m_pEventBase(NULL)
{
}

RL_Work::~RL_Work()
{
    evtimer_del( m_TimerEvent ); 
    delete m_timeout ;
    event_free(  m_TimerEvent );
    
    m_mpClientConn.clear();
    m_mpClientSock.clear();
    m_mpConnAttr.clear();
    event_base_free( m_pEventBase );
}

void CallBackReportTimer(int fd, short event, void *arg)
{
    DebugLog (<< "load report timer on");
    RL_Work* pWork = (RL_Work*)arg;
    pWork->LoadAndSend();
}

bool RL_Work::LoadAndSend( )
{
    rapidjson::Document oRoot;
    if (BuildLoadPack(oRoot) == false)
    {
        ErrLog( << "build report load msg fail" );
        return false;
    }

    bool bRet = this->SendLoad(m_sHost, m_uiPort, oRoot);
    if ( bRet == false )
    {
        ErrLog ( << "send load to policy server fail, policy_srv ip: " 
                << m_sHost << ", port: " << m_uiPort);
        return false;
    }
    else 
    {
        DebugLog(<< "send load to policy server succ");
    }

    return true;
}

bool RL_Work::BuildLoadPack(rapidjson::Document& oRoot)
{
    oRoot.SetObject();
    Document::AllocatorType& allocator = oRoot.GetAllocator();
    rapidjson::Value author;
    
    std::string sReportCmd(CMD_NO_REGISTER_SIP_REQ);
    author.SetString(sReportCmd.c_str(), sReportCmd.size(), allocator);
    oRoot.AddMember("method", author, allocator);

    static int iReq_id(100);
    oRoot.AddMember("req_id", iReq_id, allocator);
    iReq_id++;

    std::string sLocalTraceId = std::to_string(time(NULL)); 
    author.SetString(sLocalTraceId.c_str(), sLocalTraceId.size(), allocator);
    oRoot.AddMember("traceId", author, allocator);

    std::string sSpanId = std::to_string(time(NULL));
    author.SetString(sSpanId.c_str(), sSpanId.size(), allocator);
    oRoot.AddMember("spanId", author, allocator);

    int iTimeStamp = time(NULL);
    oRoot.AddMember("timestamp", iTimeStamp, allocator);
    
    ReportLoadContent::LoadContent loadCont = m_pRLContent->GetLoad(); 
    
    rapidjson::Value jsParams(rapidjson::kObjectType);
    
    rapidjson::Value jsLoad(rapidjson::kObjectType);
    jsLoad.AddMember("conn_nums", loadCont.m_iConnNums, allocator);
    jsParams.AddMember("load", jsLoad, allocator);
   
    author.SetString(loadCont.m_sLocalHost.c_str(), loadCont.m_sLocalHost.size(), allocator);
    jsParams.AddMember("ip", author, allocator);
    jsParams.AddMember("port", loadCont.m_iLocalPort, allocator);

    oRoot.AddMember("params", jsParams, allocator);
    
    StringBuffer buffer; 
    Writer<StringBuffer> writer(buffer);
    oRoot.Accept(writer);
    
    std::string sPkg = buffer.GetString();
    DebugLog( << "pkg load: " << sPkg.c_str() );

	return true;
}

bool RL_Work::SendLoad(const std::string& sIp, unsigned short usPort, rapidjson::Document& iRoot)
{
    std::stringstream ios;
    ios << sIp << ":" << usPort;
    
    std::map<std::string, tagConnFd>::iterator itClientNode;

    itClientNode = m_mpClientConn.find(ios.str());
    if (itClientNode == m_mpClientConn.end())
    {
        DebugLog( << "not find exist conn to srv: " << ios.str().c_str() << ", now create new connect");
        return AutoSend(sIp, usPort, iRoot);
    } 
    else 
    {
        unsigned int iSeq = 0;
        if (iRoot.HasMember("req_id") && iRoot["req_id"].IsInt())
        {
            iSeq = iRoot["req_id"].GetInt();
        }
        else 
        {
            static int def_seq = 0;
            iSeq = def_seq++;
            ErrLog( << "has not set req_id in send msg json, set seq val: " << iSeq);
        }

        itClientNode->second.uiSeq = iSeq;
        bool bRet = SendLoad(itClientNode->second, iRoot);
        if (bRet == false)
        {
            m_mpClientConn.erase(ios.str());
        }
    }
    return true;
}


bool RL_Work::AutoSend(const std::string& sIp, unsigned short usPort,
                       rapidjson::Document& iRoot)
{
    if (sIp.empty() || usPort <= 0)
    {
        ErrLog( << "input param ip or port invaild");
        return false;
    }

    std::shared_ptr<Sock> pClientSock(new Sock(sIp, usPort));
    int iFd = pClientSock->GetSockFd();
    if (iFd <= 0)
    {
        ErrLog( << "create connect sock fail");
        return false;
    }
    pClientSock->SetSockOpt( iFd );

    unsigned int iSeq = 0;
    if (iRoot.HasMember("req_id") && iRoot["req_id"].IsInt())
    {
        iSeq = iRoot["req_id"].GetInt();
    }
    else 
    {
        static int def_seq = 0;
        iSeq = def_seq++;
        ErrLog( << "has not set req_id in send msg json, set seq val: " << iSeq);
    }

    std::shared_ptr<ConnAttr> pConnAttr = CreateConnAttr(iFd, iSeq);
    if ( !pConnAttr )
    {
        ErrLog( << "create conn attr fail, fd: " << iFd << ", iseq: " << iSeq);
        return false;
    }
    if ( !AddReadEvent(pConnAttr) )
    {
        ErrLog( << "add read event fail on fd: " << iFd << ", seq: " << iSeq);
        DestroyConn(pConnAttr.get());
        return false;
    }
    if ( !AddWriteEvent(pConnAttr) )
    {
        ErrLog( <<"add write event fail on fd: " << iFd << ", seq: " << iSeq);
        DestroyConn(pConnAttr.get());
        return false;
    }

    if ( !pClientSock->Connect() && errno != EINPROGRESS )
    {
        ErrLog( <<"async connect to srv fail, dst ip: " << sIp.c_str() << ",dst port:" <<  usPort);
        DestroyConn(pConnAttr.get());
        return false;
    }

    if ( errno == EINPROGRESS )
    {
        pConnAttr->SetClientStatus(1);
    } 
    else 
    {
        pConnAttr->SetClientStatus(2);
    }

    std::map<int, shared_ptr<Sock> >::iterator itSock;
    itSock = m_mpClientSock.find(iFd);
    if (itSock == m_mpClientSock.end())
    {
        m_mpClientSock[iFd] = pClientSock;
    }

    std::map<int, shared_ptr<ConnAttr> >::iterator itConnAttr;
    itConnAttr = m_mpConnAttr.find( iFd );
    if ( itConnAttr == m_mpConnAttr.end() )
    {
        m_mpConnAttr[iFd] = pConnAttr;
    }

    tagConnFd  conFd;
    conFd.iFd = iFd;
    conFd.uiSeq = iSeq;

    std::stringstream ios;
    ios << sIp << ":" << usPort;
    m_mpClientConn[ios.str()] = conFd;

    pConnAttr->SetPeerNode(ios.str());

    if ( !EncodeSendData(pConnAttr, iRoot) )
    {
        ErrLog( << "encode send data fail");
        DestroyConn(pConnAttr.get());
        return false;
    }

    return true;
}

bool RL_Work::SendLoad(const tagConnFd& conFd, rapidjson::Document& iRoot)
{
    int iFd  = conFd.iFd;
    
    std::map<int, shared_ptr<ConnAttr> >::iterator itConn; 
    itConn = m_mpConnAttr.find(conFd.iFd);
    if (itConn == m_mpConnAttr.end())
    {
        m_mpClientSock.erase(iFd);
        ErrLog( << "not find conn for fd: " << conFd.iFd);
        return false;
    }

    if ( !itConn->second )
    {
        m_mpClientSock.erase(iFd);
        ErrLog( << "conn obj is null for fd: " << iFd);
        return false;
    }
    itConn->second->SetSeq(conFd.uiSeq);

    int iSeq = conFd.uiSeq;
    std::shared_ptr<ConnAttr> pConnAttr = itConn->second;
    if ( !AddWriteEvent(pConnAttr) )
    {
        ErrLog( << "add write event fail on fd: " << iFd <<  ",seq: " << iSeq);
        DestroyConn(pConnAttr.get());
        return false;
    }

    if ( !EncodeSendData(pConnAttr,iRoot) )
    {
        ErrLog( << "encode send data fail");
        DestroyConn(pConnAttr.get());
        return false;
    }
    return true;
}

std::shared_ptr<ConnAttr> RL_Work::CreateConnAttr(int iFd, unsigned int uiSeq)
{
    std::shared_ptr<ConnAttr>  oneconnAttr(new ConnAttr(iFd, uiSeq));
    return oneconnAttr;
}

bool RL_Work::EncodeSendData(std::shared_ptr<ConnAttr> pConnAttr, 
                             rapidjson::Document& iRoot)
{
    return pConnAttr->EncodeData(iRoot);
}

void RL_Work::DestroyConn(ConnAttr* pConnAttr)
{
    if (!pConnAttr)
    {
        ErrLog( << "conn attr is null, destory connect fail");
        return ;
    }
    std::string sPerrNode = pConnAttr->GetPeerNode();

    std::map<std::string, tagConnFd>::iterator itConnFd = m_mpClientConn.find(sPerrNode);
    if (itConnFd != m_mpClientConn.end())
    {
        m_mpClientConn.erase(itConnFd);
    }

    pConnAttr->DelWEvent(  );
    pConnAttr->DelREvent(  );

    int iFd = pConnAttr->GetFd();

    std::map<int, std::shared_ptr<ConnAttr> >::iterator itConnAttr;
    itConnAttr = m_mpConnAttr.find(iFd);
    if (itConnAttr != m_mpConnAttr.end())
    {
        m_mpConnAttr.erase(itConnAttr);
    }

    std::map<int, std::shared_ptr<Sock> >::iterator itSock;
    itSock = m_mpClientSock.find(iFd);
    if (itSock != m_mpClientSock.end())
    {
        m_mpClientSock.erase(itSock);
    }
    return ;
}

bool RL_Work::AddWriteEvent(std::shared_ptr<ConnAttr> pConnAttr)
{
    return pConnAttr->AddEvent(m_pEventBase, EV_WRITE, this);
}

bool RL_Work::AddReadEvent(std::shared_ptr<ConnAttr> pConnAttr)
{
    return pConnAttr->AddEvent(m_pEventBase, EV_READ|EV_PERSIST, this);
}

bool RL_Work::onStart()
{
    struct event_config *ev_config = event_config_new();
    event_config_set_flag(ev_config, EVENT_BASE_FLAG_NOLOCK);
    m_pEventBase = event_base_new_with_config(ev_config);
    event_config_free(ev_config); 

    m_timeout = new timeval;
    evutil_timerclear(m_timeout);
    m_timeout->tv_sec = m_iTms;
    m_TimerEvent = event_new( m_pEventBase, -1, EV_PERSIST, CallBackReportTimer, this);

    if( m_TimerEvent )
    {
        evtimer_add( m_TimerEvent,m_timeout );
    }

    LoadAndSend();
    return true;
}


bool RL_Work::Process()
{
    int iRet = event_base_loop(m_pEventBase, 0);
    if (iRet != 0)
    {
        ErrLog( << "report send loop exit...");
        return false;
    }
    return true;
}
