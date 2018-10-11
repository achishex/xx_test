#include "t_report_load.h"

LoadReport::LoadReport(MtsTcp* pMtsTcp): m_pLocalTcpSrv(pMtsTcp) 
{
}

LoadReport::~LoadReport()
{
}

bool LoadReport::Send(const std::string& sIp, 
                      unsigned short usPort,
                      rapidjson::Document& iRoot)
{
    std::stringstream ios;
    ios << sIp << ":" << usPort;

    auto itClientNode = m_mpClientConn.find(ios.str());
    if (itClientNode == m_mpClientConn.end())
    {
        MTS_LOG_DEBUG("not find exist conn to srv: %s, now create new connect", ios.str().c_str());
        return AutoSend(sIp, usPort, iRoot);
    } 
    else 
    {
        MTS_LOG_DEBUG("on exist connected fd: %d", itClientNode->second.iFd);
        unsigned int iSeq = 0;
        if (iRoot.HasMember("req_id") && iRoot["req_id"].IsInt())
        {
            iSeq = iRoot["req_id"].GetInt();
        }
        else 
        {
            static int def_seq = 0;
            iSeq = def_seq++;
            MTS_LOG_ERROR("has not set req_id in send msg json, set seq val: %d", iSeq);
        }

        itClientNode->second.uiSeq = iSeq;
        bool bRet = Send(itClientNode->second, iRoot);
        if (bRet == false)
        {
            MTS_LOG_ERROR("send data to policy server fail");
            m_mpClientConn.erase(ios.str());
        }
    }
    return true;
}

bool LoadReport::AutoSend(const std::string& sIp, 
                          unsigned short usPort,
                          rapidjson::Document& iRoot)
{
    if (sIp.empty() || usPort <= 0)
    {
        MTS_LOG_ERROR("input param ip or port invaild");
        return false;
    }

    std::shared_ptr<Sock> pClientSock(new Sock(sIp, usPort));
    int iFd = pClientSock->GetSockFd();
    if (iFd <= 0)
    {
        MTS_LOG_ERROR("create connect sock fail");
        return false;
    }
    pClientSock->SetSockOpt(iFd);

    unsigned int iSeq = 0;
    if (iRoot.HasMember("req_id") && iRoot["req_id"].IsInt())
    {
        iSeq = iRoot["req_id"].GetInt();
    }
    else 
    {
        static int def_seq = 0;
        iSeq = def_seq++;
        MTS_LOG_ERROR("has not set req_id in send msg json, set seq val: %d", iSeq);
    }

    std::shared_ptr<ConnAttr> pConnAttr = CreateConnAttr(iFd, iSeq);
    if (!pConnAttr)
    {
        MTS_LOG_ERROR("create conn attr fail, fd: %d, iseq: %u", iFd, iSeq);
        return false;
    }
    if ( !AddReadEvent(pConnAttr) )
    {
        MTS_LOG_ERROR("add read event fail on fd: %d, seq: %u", iFd, iSeq);
        DestroyConn(pConnAttr.get());
        return false;
    }
    if ( !AddWriteEvent(pConnAttr) )
    {
        MTS_LOG_ERROR("add write event fail on fd: %d, seq: %s", iFd, iSeq);
        DestroyConn(pConnAttr.get());
        return false;
    }

    if ( !pClientSock->Connect() && errno != EINPROGRESS )
    {
        MTS_LOG_ERROR("async connect to srv fail, dst ip: %s, dst port: %d", sIp.c_str(), usPort);
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

    auto itSock = m_mpClientSock.find(iFd);
    if (itSock == m_mpClientSock.end())
    {
        m_mpClientSock[iFd] = pClientSock;
    }

    auto itConnAttr = m_mpConnAttr.find( iFd );
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
        MTS_LOG_ERROR("encode send data fail");
        DestroyConn(pConnAttr.get());
        return false;
    }

    return true;
}

bool LoadReport::Send(const tagConnFd& conFd, rapidjson::Document& iRoot)
{
    int iFd  = conFd.iFd;
    auto itConn = m_mpConnAttr.find(conFd.iFd);
    if (itConn == m_mpConnAttr.end())
    {
        m_mpClientSock.erase(iFd);
        MTS_LOG_ERROR("not find conn for fd: %d", conFd.iFd);
        return false;
    }

    if ( !itConn->second )
    {
        m_mpClientSock.erase(iFd);
        MTS_LOG_ERROR("conn obj is null for fd: %d", iFd);
        return false;
    }
    itConn->second->SetSeq(conFd.uiSeq);

    int iSeq = conFd.uiSeq;
    std::shared_ptr<ConnAttr>  pConnAttr = itConn->second;
    if ( !EncodeSendData(pConnAttr, iRoot) )
    {
        MTS_LOG_ERROR("encode send data fail");
        DestroyConn(pConnAttr.get());
        return false;
    }

    if ( !AddWriteEvent(pConnAttr) )
    {
        MTS_LOG_ERROR("add write event fail on fd: %d, seq: %u", iFd, iSeq);
        DestroyConn(pConnAttr.get());
        return false;
    }
    MTS_LOG_DEBUG("begin to send on connected fd: %d, wait w event happen",iFd);
    return true;
}

std::shared_ptr<ConnAttr> LoadReport::CreateConnAttr( int iFd, 
                                                     unsigned int uiSeq )
{
    std::shared_ptr<ConnAttr>  oneconnAttr(new ConnAttr(iFd, uiSeq));
    return oneconnAttr;
}

bool LoadReport::EncodeSendData( std::shared_ptr<ConnAttr> pConnAttr, 
                                rapidjson::Document& iRoot )
{
    return pConnAttr->EncodeData(iRoot);
}

void LoadReport::DestroyConn(ConnAttr* pConnAttr)
{
    if (!pConnAttr)
    {
        MTS_LOG_ERROR("conn attr is null, destory connect fail");
        return ;
    }
    std::string sPerrNode = pConnAttr->GetPeerNode();

    auto itConnFd = m_mpClientConn.find(sPerrNode);
    if (itConnFd != m_mpClientConn.end())
    {
        m_mpClientConn.erase(itConnFd);
    }
    
    pConnAttr->DelWEvent(  );
    pConnAttr->DelREvent(  );
    
    int iFd = pConnAttr->GetFd();

    auto itConnAttr = m_mpConnAttr.find(iFd);
    if (itConnAttr != m_mpConnAttr.end())
    {
        m_mpConnAttr.erase(itConnAttr);
    }

    auto itSock = m_mpClientSock.find(iFd);
    if (itSock != m_mpClientSock.end())
    {
        m_mpClientSock.erase(itSock);
    }
    return ;
}

bool LoadReport::AddWriteEvent(std::shared_ptr<ConnAttr> pConnAttr)
{
    struct event_base* pBase = m_pLocalTcpSrv->GetEventBase();
    if (pBase == NULL)
    {
        MTS_LOG_ERROR("get event base is null");
        return false;
    }
    return pConnAttr->AddEvent(pBase, EV_WRITE, this);
}

bool LoadReport::AddReadEvent(std::shared_ptr<ConnAttr> pConnAttr)
{
    struct event_base* pBase = m_pLocalTcpSrv->GetEventBase();
    if (pBase == NULL)
    {
        MTS_LOG_ERROR("get event base is null");
        return false;
    }
    return pConnAttr->AddEvent(pBase, EV_READ|EV_PERSIST, this);
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
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
    MTS_LOG_DEBUG("deconstruct call: ~ConnAttr(), on fd: %d", GetFd());
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
        MTS_LOG_ERROR("encode send buf data fail");
        return false;
    } 
    return true;
}

bool ConnAttr::AddEvent(struct event_base *pEvBase, int iEvent, void *arg)
{
    LoadReport* pLoadReport = (LoadReport*)arg;

    if (pEvBase == NULL)
    {
        return false;
    }

    this->SetLoadReport(pLoadReport);

    if (EV_READ & iEvent)
    {
        MTS_LOG_DEBUG("add read event on fd: %d", m_iFd);
        event_assign(&m_stREvent, pEvBase, m_iFd, iEvent, ReadCallBack, this);
        event_add(&m_stREvent, NULL); 
    } 
    else if ( EV_WRITE & iEvent )
    {
        MTS_LOG_DEBUG("add write event on fd: %d", m_iFd);
        event_assign(&m_stWEvent, pEvBase, m_iFd, iEvent, WriteCallBack, this);
        event_add(&m_stWEvent, NULL);
    }
    else
    {  }
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
        MTS_LOG_ERROR("load report handle is null");
        return ;
    }

    if (iFd <= 0)
    {
        MTS_LOG_ERROR("write event fd is invalid, fd: %d", iFd);
        return ;
    }
    MTS_LOG_DEBUG("IoWrite(fd:%d)", iFd);

    if ( m_iClientStatus == 1 )
    {
        int iSockErr, iSockErrLen = sizeof(iSockErr);
        int iRet  = ::getsockopt( iFd, SOL_SOCKET, SO_ERROR, &iSockErr, (socklen_t *)&iSockErrLen );
        if ( iRet != 0 || iSockErr )
        {
            MTS_LOG_ERROR("getsockopt failed (errno %d)!, async connect sock err: %d ", 
                          errno, iSockErr);
            m_pLoadReport->DestroyConn(this);
            return ;
        }

        m_iClientStatus = 2;
        MTS_LOG_INFO("client has connected, fd: %d", iFd);
    }

    int iErrno = 0;
    int iNeedWtLen = m_pSendBuff->ReadableBytes();
    int iWritenLen = m_pSendBuff->WriteFD(iFd, iErrno);
    MTS_LOG_DEBUG("need to write data len: %d, has writen data len: %d", iNeedWtLen, iWritenLen);
    m_pSendBuff->Compact(8192);

    if (iWritenLen < 0)
    {
        if ( EAGAIN != iErrno && EINTR != iErrno )
        {
            MTS_LOG_ERROR("write data err on fd: %d, err msg: %s", iFd, strerror(errno));
            m_pLoadReport->DestroyConn(this);
            return ;
        }

        bool bRet = AddEvent( m_pLoadReport->GetEventBase(), EV_WRITE, m_pLoadReport );
        if (bRet == false)
        {
            MTS_LOG_ERROR("add write event again fail");
        }
    }
    else if (iWritenLen >0)
    {
        if (iWritenLen == iNeedWtLen)
        {
            MTS_LOG_DEBUG("write data to peer node complete,fd: %d,sendlen: %d", 
                          iFd, iNeedWtLen);
            return ;
        }
        else 
        {
            MTS_LOG_DEBUG("need to write len: %d, has writen len: %d, on fd:%d",
                          iNeedWtLen, iWritenLen, iFd);
            AddEvent( m_pLoadReport->GetEventBase(), EV_WRITE, m_pLoadReport );
        }
    }
    else
    {
        MTS_LOG_DEBUG("has no more data to send on fd: %d", iFd);
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
    MTS_LOG_DEBUG("recv data from fd: %d, data len: %d", iFd, iReadLen);

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
            MTS_LOG_ERROR( "decode recv data err, on fd: %d", iFd );
            m_pLoadReport->DestroyConn(this);
        }
        else
        {
            return ;
        }
    }
    else if (iReadLen == 0)
    {
        MTS_LOG_INFO("peer node active close this conn, err msg: %s", strerror(iErrno));
        m_pLoadReport->DestroyConn(this);
        return ;
    }
    else 
    {
        if (EAGAIN != iErrno && EINTR != iErrno)
        {
            MTS_LOG_ERROR("recv data has err, err msg: %s", strerror(iErrno));
            m_pLoadReport->DestroyConn(this);
            return ;
        }
    }
}

bool ConnAttr::DoBusiProc(rapidjson::Document& retDoc)
{
    if (retDoc.HasParseError())
    {
        MTS_LOG_ERROR("parse response json fail");
        return false;
    }

    int iSeq = 0;
    if (retDoc.HasMember("req_id") && retDoc["req_id"].IsInt())
    {
        iSeq = retDoc["req_id"].GetInt();
    }

    if (m_iSeq != iSeq)
    {
        MTS_LOG_ERROR("recv response seq: %d !=  req seq: %d", iSeq, m_iSeq);
        return false;
    }

    int iCode = 0;
    if ( retDoc.HasMember("code") && retDoc["code"].IsInt() )
    {
        iCode = retDoc["code"].GetInt();
    }
    std::string sMsg = retDoc["msg"].GetString();

    if ( retDoc.HasMember("msg") && retDoc["msg"].IsString() )
    {
        if (iCode != 0)
        {
            MTS_LOG_ERROR("resp code: %d, err msg: %s", iCode, sMsg.c_str());
            return false;
        }
        else
        {
            MTS_LOG_DEBUG("resp code: %d, msg: %s", iCode, sMsg.c_str());
            return true;
        }
    }
    return true;
}

