#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"

#include "AsyncProcessorMessage.hxx"
#include "monkeys/PushIotServer.hxx"
#include "RequestContext.hxx"
#include "Proxy.hxx"

#include "resip/stack/SipStack.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "resip/stack/ExtensionHeader.hxx"

#include "protocol/proto_inner.h"
#include "QValueTarget.hxx"
#include "ReplaceSdpByMedia.hxx"

#include <sys/time.h>
#include <map>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;
using namespace rapidjson;

class ReleaseMtsPortAsyncMsg: public AsyncProcessorMessage
{
 public:
  ReleaseMtsPortAsyncMsg(AsyncProcessor& proc,
                  const resip::Data& tid,
                  TransactionUser* passedtu)
      :AsyncProcessorMessage(proc,tid,passedtu),
      m_MtsIp(""), m_MtsPort(0), m_iResult(0)
  {
  }
  virtual ~ReleaseMtsPortAsyncMsg()
  { }

  std::string   m_MtsIp;
  unsigned int  m_MtsPort;
  resip::Data   m_CallId;
  int           m_iResult;
};

class NotifyAsyncMesg: public AsyncProcessorMessage
{
 public:
  NotifyAsyncMesg(AsyncProcessor& proc,
                  const resip::Data& tid,
                  TransactionUser* passedtu,
                  const resip::ContactList& vContact)
      : AsyncProcessorMessage(proc, tid, passedtu),
      m_vdstContact(vContact),
      m_QueryResult(0)
    { }
  virtual ~NotifyAsyncMesg()
  { }

  //
  resip::ContactList m_vdstContact;
  int m_QueryResult;
};

class NotifyTimerMsg: public ProcessorMessage 
{
 public:
  NotifyTimerMsg(const repro::Processor& proc,
                 const resip::Data& tid,
                 resip::TransactionUser* tuPassed)
      : ProcessorMessage(proc, tid, tuPassed), m_QueryResult( 0 )
  { }
  virtual ~NotifyTimerMsg() { }

  NotifyTimerMsg(const NotifyTimerMsg& item)
      :ProcessorMessage(item)
  {
      m_QueryResult = item.m_QueryResult;
  }

  virtual NotifyTimerMsg* clone() const 
  {
      return new NotifyTimerMsg(*this);
  }

  virtual EncodeStream& encode(EncodeStream& ostr) const 
  { 
      ostr << "NotifyTimerMsg(tid="<<mTid<<"): ";
      return ostr; 
  }

  virtual EncodeStream& encodeBrief(EncodeStream& ostr) const 
  { 
      return encode(ostr);
  }

  ///////////////////////
  int m_QueryResult;
  Data m_CallId;
};


PushIotServer::PushIotServer(ProxyConfig& config,
	Dispatcher* asyncDispatcher,
	resip::RegistrationPersistenceManager& store) :
	AsyncProcessor("PushIotServer", asyncDispatcher),
	mDefaultErrorBehavior("500, Server Internal DB Error"),
	mSipConServerIP(config.getConfigData("PolicyServerIP", "10.101.70.52")),
    mSipConServerPort(config.getConfigInt("PolicyServerPort", 7878)),
	mMediaServerIP(config.getConfigData("MediaIP", "10.101.70.52")),
    mMediaServerPort(config.getConfigInt("MediaPort", 7878)),
	mIotServerIP(config.getConfigData("IotServerIP", "10.101.70.52")),
    mIotServerPort(config.getConfigInt("IotServerPort", 7878)),
    mIotAuthPort(config.getConfigInt("IotAuthPort", 7878)),
    mAuthFlag(config.getConfigInt("AuthFlag", 0)),
    mAutuEncryFlag(config.getConfigData("AutuEncryFlag", "false").c_str()),
    mStore(store),
    mLocalSipUri( config.getConfigUri("RecordRouteUri", Uri()) ),
    m_ProxyConfig( config )
    {
    }


PushIotServer::~PushIotServer()
{
}

short
PushIotServer::parseActionResult(const Data& result, Data& redirectReason)
{
	Data redirectStatusCode;
	ParseBuffer pb(result);
	const char* anchor = pb.position();
	pb.skipToChar(',');
	pb.data(redirectStatusCode, anchor);
	if (pb.position()[0] == ',')
	{
		pb.skipChar();
		pb.skipWhitespace();
		anchor = pb.position();
		pb.skipToEnd();
		pb.data(redirectReason, anchor);
	}
	return (short)redirectStatusCode.convertInt();
}


Processor::processor_action_t
PushIotServer::applyActionResult(RequestContext &rc, const Data& actionResult, const resip::NameAddr& serverAddr)
{
	if (!actionResult.empty())
	{
		Data redirectReason;
		short redirectStatusCode = parseActionResult(actionResult, redirectReason);

		if (redirectStatusCode >= 400 && redirectStatusCode < 600)
		{
			// error 
			SipMessage response;
			InfoLog(<< "Request is failed - responding with a " << redirectStatusCode);
			Helper::makeResponse(response, rc.getOriginalRequest(), redirectStatusCode);
			rc.sendResponse(response);
			return SkipThisChain;
		}
		else if (redirectStatusCode == 301 || redirectStatusCode == 302)
		{
			SipMessage response;
			InfoLog(<< "Request is successed - responding with a " << redirectStatusCode);

            /* 增加应答的私有字段 */
            resip::Uri inputUri = rc.getOriginalRequest().header(h_RequestLine).uri().getAorAsUri(rc.getOriginalRequest().getSource().getType());
			//resip::Uri inputUri = rc.getOriginalRequest().header(h_From).uri().getAorAsUri(rc.getOriginalRequest().getSource().getType());
		    //!RjS! This doesn't look exception safe - need guards
		    mStore.lockRecord(inputUri);

		    resip::ContactList contacts;
		    mStore.getContacts(inputUri,contacts);			

            mStore.unlockRecord(inputUri);

			resip::ContactInstanceRecord contact;
            if(contacts.size() > 0)
            {
                for(resip::ContactList::iterator i  = contacts.begin(); i != contacts.end(); ++i)
                {
                    contact = *i;                
                    InfoLog (<< *this << " Local contact " << contact.mPLocalContact <<
                        " with tuple " << contact.mReceivedFrom);

					ExtensionHeader h_LocalContact("P-Local-Contact");
					response.header(h_LocalContact).push_front(StringCategory(contact.mPLocalContact));
					//break;
                }
            }
			
			Helper::makeResponse(response, rc.getOriginalRequest(), redirectStatusCode, serverAddr);
			rc.sendResponse(response);
			return SkipThisChain;
		}
	}

	// 
	DebugLog(<< "Should not get here");
	return Continue;
}

Processor::processor_action_t
PushIotServer::process(RequestContext &rc)
{
	DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << rc);

	Message *message = rc.getCurrentEvent();

	PushIotServerAsyncMessage *async_msg_first_step = dynamic_cast<PushIotServerAsyncMessage*>(message);
    NotifyAsyncMesg* async_notify_second_step       = dynamic_cast<NotifyAsyncMesg*>(message);
    NotifyTimerMsg* tmr_msg_third_step              = dynamic_cast<NotifyTimerMsg*>(message);
    ReleaseMtsPortAsyncMsg*  tmr_release_port_step  = dynamic_cast<ReleaseMtsPortAsyncMsg*>(message);

	if (async_msg_first_step)
	{
		if (async_msg_first_step->mQueryResult == 0)  // If query was successful, then get query result
		{
            //add timer and store from_to<->process into proxy map
            if ( false == SetCurReqContextProcess(rc) )
            {
                ErrLog (<< "store fromto and reqcontext,process map fail, tid: " 
                        << rc.getTransactionId() );
                return SkipThisChain;
            }

            PostWaitNotifymMsgTimer(rc, async_msg_first_step->mCall_ID);

            //add callid <-> mts relay ports map
            SetMtsPortWithCallId(rc, *async_msg_first_step);  
			
            return WaitingForEvent;
		}
		else
		{
			InfoLog(<< "get sipserver failed: Result=" << async_msg_first_step->mQueryResult
                    <<"error message is :"<<async_msg_first_step->mErrorMessage);
            return Continue;
		}
	}
    else if(async_notify_second_step)
    {
        DebugLog( << "recv notify async msg after async process" );
        rc.m_NotifyStatus = Recv_Notify_Status;
        bool bRet = MakeTargetNodes( async_notify_second_step, rc );        
        if (bRet == false)
        {
            ErrLog( << "make target node fail" );
            return Processor::SkipThisChain;
        }

        //motify request sdp contents 
        //add by achilsh, replace sdp info by media server info
        resip::SipMessage* msg = &(rc.getOriginalRequest()); //dynamic_cast<SipMessage*>(message);
        Contents* pReqContent  = msg->getContents();
        Proxy& proxyHandle     = rc.getProxy();
        resip::Data dCallId    = msg->header(h_CallID).value();
        MediaPortSource* pMediaHandle =  proxyHandle.GetMtsPortsByCallId( dCallId );
        if ( !pMediaHandle || !pReqContent )
        {
            ErrLog( << "callid: " << dCallId << ", mediaportsource handle is empty!");
            return Continue;
        }

        //is invite request 
        SdpReplaceMedia replaceMedia(pReqContent, pMediaHandle);
        if (false == replaceMedia.Replace())
        {
            ErrLog( << "replace sdp media info fail, callid: " <<  dCallId);
        }

        //add statistic receviced invite times, add by achilsh
        proxyHandle.GetReportLoadContent()->IncrConnNums();

        //modify contact value by request msg via's rport value
        ModifyContactByReqViaRport( rc );

        DebugLog( << "invite, replace sdp media info succ, callid: " << dCallId);
        DebugLog( << "invite request msg: \r\n" << *msg);

        return Continue;
    }
    else if (tmr_msg_third_step)
    {
        if (rc.m_NotifyStatus == Sent_PushMsg_Status || 
            rc.m_NotifyStatus == Init_Status)
        {
            ErrLog( << "tid: " << rc.getTransactionId() << ", recv notify timer tmout msg" );
            Proxy* proxyHandle      = &(rc.getProxy());

            resip::Data to_data     = rc.getOriginalRequest().header(h_To).uri().getAor(); 
            resip::Data from_data   = rc.getOriginalRequest().header(h_From).uri().getAor(); 
            from_data += to_data;

            proxyHandle->DelNotifyInviteHandle( from_data );

            // del mts ports  by callid
            resip::Data rc_callId = rc.getOriginalRequest().header(h_CallID).value();
            DebugLog( << "rc_callid: " << rc_callId << ", tmr_callid: " << tmr_msg_third_step->m_CallId );

            MediaPortSource* pMtsNode = proxyHandle->GetMtsPortsByCallId(rc_callId);
            if (pMtsNode == NULL)
            {
                ErrLog( << "not find mts node by callid: " << rc_callId );
                return SkipThisChain;
            }
            proxyHandle->DelMtsRelayPorts(rc_callId);

            PostReleaseMtsPortMsg(rc, pMtsNode->m_MtsIp, pMtsNode->m_mtsPort);
            return WaitingForEvent; 
        }

        DebugLog ( << "after recv notify mesg, timer msg arrive, tid: " << rc.getTransactionId() );
        //if more than one notify, from_to_data is null
        return Continue;
    }
    else if (tmr_release_port_step)
    {
        ErrLog( << "has sent release mts port done. end sip call, callId: "
               << rc.getOriginalRequest().header(h_CallID).value()
               << ", async_ret: " << tmr_release_port_step->m_iResult );
        return SkipThisChain;
    }
	else
	{
		// Dispatch async
		PushIotServerAsyncMessage* async_msg = new PushIotServerAsyncMessage(*this, 
                                                                             rc.getTransactionId(), 
                                                                             &rc.getProxy());
        async_msg->mCall_ID    = rc.getOriginalRequest().header(h_CallID).value();//获取CALL_ID
		async_msg->mFrom       = rc.getOriginalRequest().header(h_From).uri().getAorNoReally();//获取FROM
		async_msg->mTo         = rc.getOriginalRequest().header(h_To).uri().getAorNoReally();//获取TO
        /*取私有字段在扩展头里*/
        resip::ExtensionHeader h_pAuthToken("P-Auth-Token");
        if(rc.getOriginalRequest().exists(h_pAuthToken))
        {
		    async_msg->mAuthToken = rc.getOriginalRequest().header(h_pAuthToken).begin()->value();
		    InfoLog(<< "Auth Token is:" << async_msg->mAuthToken.c_str() << " Get auth token!");
        }
        std::auto_ptr<ApplicationMessage> async_msg_first_step(async_msg);
		mAsyncDispatcher->post(async_msg_first_step);

		return WaitingForEvent;
	}
}

//异步处理流程（处理三个同步消息）
bool
PushIotServer::asyncProcess(AsyncProcessorMessage* msg)
{
    do 
    {
        NotifyAsyncMesg* async = dynamic_cast<NotifyAsyncMesg*>(msg);
        if (async)
        {
            DebugLog ( << "recv aysnc mesg  for notify" );
            return true;
        }

        ReleaseMtsPortAsyncMsg* releaseMtsPortMsg = dynamic_cast<ReleaseMtsPortAsyncMsg*>(msg);
        if (releaseMtsPortMsg)
        {
            const std::string& sip  = releaseMtsPortMsg->m_MtsIp;
            unsigned int port       = releaseMtsPortMsg->m_MtsPort;
            resip::Data& callId     = releaseMtsPortMsg->m_CallId;
            DebugLog( << "recv aync release mts port msg, begin to release port," 
                     << "callid: " << callId << ", mts ip: " << sip << ", port: "
                     << port);
            bool bret = ReleaseRemoteMediaPort( callId, sip, port );
            if (bret == false)
            {
                releaseMtsPortMsg->m_iResult = -1;
            }
            return true;
        }
    } while(0);

	PushIotServerAsyncMessage* async = dynamic_cast<PushIotServerAsyncMessage*>(msg);
	resip_assert(async);
    
	async->mQueryResultData = Data("Start Deal internal Message!");
	async->mQueryResult = 0;


    /*1.先向IOT服务发起鉴权校验*/
	std::auto_ptr<ConServerSynClient> IotAuthClient(new ConServerSynClient(mIotServerIP.c_str(),mIotAuthPort));
	if(!IotAuthClient->ConnectServer())
	{
	    ErrLog(<< "Connect Iot Auth Server Failed");
        async->mQueryResult = -1;
        async->mErrorMessage = "Connect Iot Auth Server Failed";
        return false;
	}
    std::string strFrom      = async->mFrom.c_str();
	std::string strTo        = async->mTo.c_str();
    std::string strAuthToken = async->mAuthToken.c_str();  //
    DebugLog(<<"strFrom = "<<strFrom<<", strTo = "<<strTo<<", strAuthToken = "<<strAuthToken);
    if (false == SendAuth2Iot(IotAuthClient,strFrom,strTo,strAuthToken))
    {
        ErrLog(<< "Iot return Auth Failed");
        async->mErrorMessage = "Iot return Auth Failed";
        if (mAuthFlag == 1)
        {
            async->mQueryResult = -2;
            return false;
        }
    }
	

	/*2.第二向策略服务器取媒体服务器IP and port*/
	std::auto_ptr<ConServerSynClient> plySynClient(new ConServerSynClient(mSipConServerIP.c_str(),mSipConServerPort));
	if(!plySynClient->ConnectServer())
	{
	    ErrLog(<< "Connect Policy Server Failed");
        async->mQueryResult = -3;
        async->mErrorMessage = "Connect Policy Server Failed";
        return false;
	}
    std::string strIp; int iPort;
    if (false == GetMediaIP_Port(plySynClient,strIp,iPort))
    {
        ErrLog( << "can't get media ip: " << strIp << ", port: " << iPort );
        async->mQueryResult = -4;
        async->mErrorMessage = "get media ip and port Failed";
        return false;
    }
    DebugLog(<<"ip = "<<strIp<<", port = "<<iPort);	

    async->mMediaPortData.m_MtsIp = strIp;
    async->mMediaPortData.m_mtsPort = iPort;
	
    /*3.向媒体服务器获取端口*/
    std::auto_ptr<ConServerSynClient> MediaClient(new ConServerSynClient(strIp, iPort));
	if (!MediaClient->ConnectServer())
    {
        ErrLog(<< "Connect Media Server Failed");
        async->mQueryResult = -5;
        async->mErrorMessage = "Connect Media Server Failed";
        return false;
    }
	std::string callId = async->mCall_ID.c_str();
	if ( false == GetMediaResourePort(MediaClient, callId, async) )
    {
        ErrLog( << "get media relay ports Failed");
        async->mQueryResult = -6;
        async->mErrorMessage = "get media relay ports Failed";
        return false;
    }

	/*4.向IOT发送需要推送的消息*/
	std::auto_ptr<ConServerSynClient> IotPushClient(new ConServerSynClient(mIotServerIP.c_str(),mIotServerPort));
	if(!IotPushClient->ConnectServer())
	{
	    ErrLog(<< "Connect Iot Push Server Failed");
        async->mQueryResult = -7;
        async->mErrorMessage = "Connect Iot Push Server Failed";
        return false;
	}
	SendPush2Iot(IotPushClient,strFrom,strTo);
	return true;
}

//JSON转字符串
std::string
PushIotServer::ConverJason2String(rapidjson::Document &inroot)
{
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    inroot.Accept(writer);
    std::string sResultData = buffer.GetString();

    return sResultData;
}



//拼接获取流媒体IP的JSON数据 
bool PushIotServer::GetMediaIP_Port(std::auto_ptr<ConServerSynClient> policyCli,std::string& strMediaIp,int & iport )
{
	rapidjson::Document oroot;
	oroot.SetObject();
	rapidjson::Document::AllocatorType &allocator = oroot.GetAllocator();

    std::string method = CMD_NO_GET_MTS_REQ;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    int timestamp = tv.tv_sec;
    //int reqID = 123;
	std::string traceId = "88";
	std::string spanId  = "88";

	rapidjson::Value method_;
    method_.SetString( method.c_str() , method.length() , allocator ) ;
    oroot.AddMember("method", method_, allocator);
    oroot.AddMember("timestamp", timestamp, allocator);
	oroot.AddMember("req_id", timestamp, allocator);

	rapidjson::Value traceid_;
    traceid_.SetString( traceId.c_str() , traceId.length() , allocator ) ;
    oroot.AddMember("traceId", traceid_, allocator);

	rapidjson::Value spanid_;
    spanid_.SetString( spanId.c_str() , spanId.length() , allocator ) ;
    oroot.AddMember("spanId", spanid_, allocator);

    string resultData = "";

    InfoLog(<< "start Send Policy Server....");
    
    policyCli->DoWrite(oroot);//发送消息给策略服务器
    rapidjson::Document rspJsonData;

    resultData = ConverJason2String(oroot);

    InfoLog(<< "req ConServer data =" << resultData);
    //解析策略服务器的返回JSON拿对应的数据
    if (false == policyCli->DoRead(rspJsonData))
    {
        ErrLog( << "read policy server response fail" );
        return false;
    }

    if (false == ParseMediaIP(rspJsonData,strMediaIp,iport))
    {
        ErrLog( << "read policy server return media ip_port  fail" );
        return false;
    }
    InfoLog(<< "Get media serverip:" << strMediaIp << ", Get media serverport: "<< iport );
    return true;
}

//拼接获取流媒体分配的端口的JSON数据
bool 
PushIotServer::GetMediaResourePort(std::auto_ptr<ConServerSynClient> mediaCli,
                                         std::string callId,
                                         PushIotServerAsyncMessage* pAsyncMsg)
{
	rapidjson::Document oroot;
	oroot.SetObject();
	rapidjson::Document::AllocatorType &allocator = oroot.GetAllocator();

    std::string method = CMD_NO_ALLOCATE_RELAY_PORT_REQ;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int timestamp = tv.tv_sec;

	std::string traceId = "77";
	std::string spanId  = "77";

	rapidjson::Value method_;
    method_.SetString( method.c_str() , method.length() , allocator ) ;
    oroot.AddMember("method", method_, allocator);

    oroot.AddMember("timestamp", timestamp, allocator);
	
	oroot.AddMember("req_id", timestamp, allocator);

	rapidjson::Value traceid_;
    traceid_.SetString( traceId.c_str() , traceId.length() , allocator ) ;
    oroot.AddMember("traceId", traceid_, allocator);

	rapidjson::Value spanid_;
    spanid_.SetString( spanId.c_str() , spanId.length() , allocator ) ;
    oroot.AddMember("spanId", spanid_, allocator);
	
	rapidjson::Document params;
	params.SetObject();
	rapidjson::Document::AllocatorType &sallocator = params.GetAllocator();

	rapidjson::Value params_;
    params_.SetString( callId.c_str() , callId.length() , sallocator ) ;
    params.AddMember("session_id", params_, sallocator);//获取会话ID为callid
	oroot.AddMember("params", params, sallocator);

    InfoLog(<< "start Send Media Server....");
    //发送消息给mts
    if ( false == mediaCli->DoWrite(oroot) )
    {
        ErrLog (<< "fail send to get media port ");
        return false;
    }

	string resultData = "";
    resultData = ConverJason2String(oroot);

    InfoLog(<< "req ConServer data =" << resultData);
    rapidjson::Document rspJsonData;
    
    //解析策略服务器的返回JSON拿对应的数据
    if ( false == mediaCli->DoRead(rspJsonData) )
    {
        ErrLog ( << "recv get media port response fail"  );
        return false;
    }

    if (false == ParseMediaResourcePort(rspJsonData, pAsyncMsg->mMediaPortData))
    {
        ErrLog ( << "get resource port fail!"  );
        return false;
    }

    InfoLog(<< "Get media info: " << pAsyncMsg->mMediaPortData);
    return true;
}


 /*组装发送给IOT鉴权推送的JSON*/
bool PushIotServer::SendAuth2Iot(std::auto_ptr<ConServerSynClient> authCli,std::string strFrom,std::string strTo,std::string strAuthToken)
{
    rapidjson::Document oroot;
	oroot.SetObject();
	rapidjson::Document::AllocatorType &allocator = oroot.GetAllocator();

    std::string uuid = "2001";
    rapidjson::Value UUID;
    UUID.SetString( uuid.c_str() , uuid.length() , allocator ) ;
    oroot.AddMember("uuid", UUID, allocator);

    rapidjson::Value ENCRY;
    ENCRY.SetString( mAutuEncryFlag.c_str() , mAutuEncryFlag.length() , allocator ) ;
    oroot.AddMember("encry", ENCRY, allocator);

    rapidjson::Document content;
	content.SetObject();
	rapidjson::Document::AllocatorType &hallocator = content.GetAllocator();

    std::string method = CMD_NO_AUTH_PUSH_IOT;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int timestamp = tv.tv_sec;

	rapidjson::Value method_;
    method_.SetString( method.c_str() , method.length() , hallocator ) ;
    content.AddMember("method", method_, hallocator);

    content.AddMember("timestamp", timestamp, hallocator);
	content.AddMember("req_id", timestamp, hallocator);
	std::string traceId = "66";
	std::string spanId  = "66";
	
    rapidjson::Value traceid_;
    traceid_.SetString( traceId.c_str() , traceId.length() , hallocator ) ;
    content.AddMember("traceId", traceid_, hallocator);

	rapidjson::Value spanid_;
    spanid_.SetString( spanId.c_str() , spanId.length() , hallocator ) ;
    content.AddMember("spanId", spanid_, hallocator);

    rapidjson::Document params;
	params.SetObject();
	rapidjson::Document::AllocatorType &sallocator = params.GetAllocator();

	rapidjson::Value srcAVURL;
    srcAVURL.SetString( strFrom.c_str() , strFrom.length() , sallocator ) ;
    params.AddMember("srcAVURL", srcAVURL, sallocator);//FROM

	rapidjson::Value dstAVURL;
    dstAVURL.SetString( strTo.c_str() , strTo.length() , sallocator ) ;
    params.AddMember("dstAVURL", dstAVURL, sallocator);//To

	rapidjson::Value authtoken;
    authtoken.SetString( strAuthToken.c_str() , strAuthToken.length() , sallocator ) ;
    params.AddMember("token", authtoken, sallocator);//Token

   	content.AddMember("params", params, sallocator);
	oroot.AddMember("content", content, hallocator);

    InfoLog(<< "start Send IOT Auth Server....");
    
    if ( false == authCli->DoWrite(oroot) )
    {
        ErrLog ( << "send Iot msg to server fail" );
        return false;
    }

    std::string resultData = ConverJason2String(oroot);
    DebugLog(<< "req Iot Auth data =" << resultData);
    
    rapidjson::Document rspJsonData;
    if ( false == authCli->DoRead(rspJsonData) )
    {
        ErrLog( << "recv Iot Auth msg server response fail");
        return false;
    }

   return ParseIotAuthReturnPackage(rspJsonData);
}

//拼接发送给IOT的JSON数据
void 
PushIotServer::SendPush2Iot(std::auto_ptr<ConServerSynClient> inviteCli,std::string strFrom,std::string strTo)
{
	rapidjson::Document oroot;
	oroot.SetObject();
	rapidjson::Document::AllocatorType &allocator = oroot.GetAllocator();

    std::string method = CMD_NO_PUSH_IOT;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int timestamp = tv.tv_sec;

	rapidjson::Value method_;
    method_.SetString( method.c_str() , method.length() , allocator ) ;
    oroot.AddMember("method", method_, allocator);

    oroot.AddMember("timestamp", timestamp, allocator);
	oroot.AddMember("req_id", timestamp, allocator);
	std::string traceId = "66";
	std::string spanId  = "66";
	
    rapidjson::Value traceid_;
    traceid_.SetString( traceId.c_str() , traceId.length() , allocator ) ;
    oroot.AddMember("traceId", traceid_, allocator);

	rapidjson::Value spanid_;
    spanid_.SetString( spanId.c_str() , spanId.length() , allocator ) ;
    oroot.AddMember("spanId", spanid_, allocator);
	std::string resultData = "";

	rapidjson::Document params;
	params.SetObject();
	rapidjson::Document::AllocatorType &sallocator = params.GetAllocator();

    std::string strIotMethond = "sip_invite"; 
    rapidjson::Value Sip_invite;
    Sip_invite.SetString( strIotMethond.c_str() , strIotMethond.length() , sallocator ) ;
    params.AddMember("method", Sip_invite, sallocator);//FROM


    rapidjson::Document content;
	content.SetObject();
	rapidjson::Document::AllocatorType &hallocator = content.GetAllocator();

	rapidjson::Value srcAVURL;
    srcAVURL.SetString( strFrom.c_str() , strFrom.length() , hallocator ) ;
    content.AddMember("srcAVURL", srcAVURL, hallocator);//FROM

	rapidjson::Value dstAVURL;
    dstAVURL.SetString( strTo.c_str() , strTo.length() , hallocator ) ;
    content.AddMember("dstAVURL", dstAVURL, hallocator);//To

    const resip::Uri& uriLocalSip = mLocalSipUri.uri();
    if (uriLocalSip.host().empty())
    {
        ErrLog( <<"local sip host not config by field: RecordRouteUri " );
        return ;
    }
    resip::Data data_uri_sip = uriLocalSip.toString();
	std::string strLocalSipAddr(data_uri_sip.c_str());
    DebugLog ( << "sipURL: " << strLocalSipAddr );

	rapidjson::Value sipURL;
    sipURL.SetString( strLocalSipAddr.c_str() , strLocalSipAddr.length() , hallocator ) ;
    content.AddMember("sipURL", sipURL, hallocator);//To
   	params.AddMember("content", content, hallocator);
	oroot.AddMember("params", params, sallocator);

    InfoLog(<< "start Send IOT Server....");
    
    if ( false == inviteCli->DoWrite(oroot) )
    {
        ErrLog ( << "send push msg to server fail" );
        return ;
    }

    resultData = ConverJason2String(oroot);
    DebugLog(<< "req ConServer data =" << resultData);
    
    rapidjson::Document rspJsonData; 
    if ( false == inviteCli->DoRead(rspJsonData) )
    {
        ErrLog( << "recv push msg server response fail"  );
        return ;
    }

    ParseIotPushReturnPackage(rspJsonData);
}


//解析JSON拿流媒体IP跟端口
 bool PushIotServer::ParseMediaIP(rapidjson::Document &inroot,std::string &strMediaIp,int& iport)
{
    do
	{
	    if (inroot.HasParseError() )
        {
            break;
            ErrLog(<<"The policy server return json parse fail");
            return false;
        }
	    if (!inroot.IsObject() )
        {
            break;
            ErrLog(<<"The policy server return code is not an object");
            return false;
        }
        std::string resultData = ConverJason2String(inroot);
        DebugLog(<< "rsp ConServer data from Policy server is: " << resultData);
        if(inroot.HasMember("code")  && inroot["code"].IsInt() && inroot["code"].GetInt() == 0)
        {
            if ( inroot.HasMember("result") && inroot["result"].IsObject() )
            {
                rapidjson::Value info_object(rapidjson::kObjectType);
                info_object.SetObject();
                info_object = inroot["result"].GetObject();
                if(info_object.HasMember("ip")&&info_object["ip"].IsString())
                {
                    strMediaIp = info_object["ip"].GetString();
                }
                if(info_object.HasMember("port")&&info_object["port"].IsInt())
                {
                    iport = info_object["port"].GetInt();
                }		   
            }
            else
            {
                ErrLog(<<"The  policy server return package don't contain result!");
                return false;
            }	
        }
        else
        {
            ErrLog(<<"The policy server return code is invalid!"<<inroot["code"].GetInt());
            return false;
        }	
	}while(0);

    return true;
}


//解析JSON拿流媒体资源端口
 bool PushIotServer::ParseMediaResourcePort(rapidjson::Document &inroot,MediaPortSource &AllPortData)
{
    do
	{
	    if (inroot.HasParseError() )
        {
            break;
            ErrLog(<<"The media server return json parse fail!");
            return false;
        }
	    if (!inroot.IsObject() )
        {
            break;
            ErrLog(<<"The media server return code is not an object");
            return false;
        }
        std::string resultData = ConverJason2String(inroot);
        DebugLog(<< "rsp ConServer data from media server is: " << resultData);
        if(inroot.HasMember("code") && inroot["code"].IsInt() && inroot["code"].GetInt() == 0)
        {
            if ( inroot.HasMember("result") && inroot["result"].IsObject() )
            {
                rapidjson::Value info_object(rapidjson::kObjectType);
                info_object.SetObject();
                info_object = inroot["result"].GetObject();
                if(info_object.HasMember("port")&&info_object["port"].IsObject())
                {
                        AllPortData.relay_caller_audio_rtp_port      = info_object["port"]["relay_caller_audio_rtp_port"].GetInt();
                        AllPortData.relay_caller_audio_rtcp_port     = info_object["port"]["relay_caller_audio_rtcp_port"].GetInt();
                        AllPortData.relay_caller_video_rtp_port      = info_object["port"]["relay_caller_video_rtp_port"].GetInt();
                        AllPortData.relay_caller_video_rtcp_port     = info_object["port"]["relay_caller_video_rtcp_port"].GetInt();
                        AllPortData.relay_callee_audo_io_rtp_port    = info_object["port"]["relay_callee_audo_io_rtp_port"].GetInt();
                        AllPortData.relay_callee_audo_io_rtcp_port   = info_object["port"]["relay_callee_audo_io_rtcp_port"].GetInt();
                        AllPortData.relay_callee_video_io_rtp_port   = info_object["port"]["relay_callee_video_io_rtp_port"].GetInt();
                        AllPortData.relay_callee_video_io_rtcp_port  = info_object["port"]["relay_callee_video_io_rtcp_port"].GetInt();
                }		   
            }else{
                ErrLog(<<"The return package media server don't contain result!");
                return false;
            }
        }
        else
        {
            ErrLog(<<"The media server return code is invalid!"<<inroot["code"].GetInt());
            return false;
        }		
	}while(0);

    return true;
}


//解析IOT的Auth JSON回包
bool PushIotServer::ParseIotAuthReturnPackage(rapidjson::Document &inroot)
{
    bool bret = false;
    int icode = -1;
     do
	{
	    if (inroot.HasParseError() )
        {
            break;
            ErrLog(<<"The Iot server return json parse fail");
            return bret;
        }
	    if (!inroot.IsObject() )
        {
            break;
            ErrLog(<<"The Iot server return code is not an object");
            return bret;
        }
        std::string resultData = ConverJason2String(inroot);
        DebugLog(<< "rsp Auth data from Iot server is: " << resultData);
        if ( inroot.HasMember("content") && inroot["content"].IsObject() )
        {
           rapidjson::Value info_object(rapidjson::kObjectType);
           info_object.SetObject();
           info_object = inroot["content"].GetObject();
		   if(info_object.HasMember("code")&&info_object["code"].IsString())
		   {
		       icode = info_object["code"].GetInt();
               if (icode == 0)
               {
                   bret=true;
               }
		   }	   
        }		
	}while(0);

    return bret;
}

//解析IOT的JSON回包
void PushIotServer::ParseIotPushReturnPackage(rapidjson::Document &inroot)
{
    do
	{
	    if (inroot.HasParseError() )
        {
            break;
        }
	    if (!inroot.IsObject() )
        {
            break;
        }
        std::string resultData = ConverJason2String(inroot);
        DebugLog(<< "rsp ConServer data from Iot server is: " << resultData);
        if ( inroot.HasMember("code") && inroot["code"].IsInt()  )
        {
            if (inroot["code"].GetInt() != 0)
            {
                ErrLog( << "Iot return  fail," 
                <<inroot["code"].GetInt() );
               return;
            }
                                        
       }
	}while(0);

    return;
}

bool PushIotServer::NotifyMsgPost(const resip::ContactList& vdstContact, RequestContext& rc)
{
    std::auto_ptr<ApplicationMessage> async_second_step(new NotifyAsyncMesg( *this,
                                                                            rc.getTransactionId(),
                                                                            &rc.getProxy(),
                                                                            vdstContact) );
    mAsyncDispatcher->post( async_second_step );
    DebugLog( << "post notify async msg, tid:  " << rc.getTransactionId() );
    return true;
}

bool PushIotServer::SetCurReqContextProcess(RequestContext& rc)
{
    Proxy* proxyHandle      = &(rc.getProxy());
    resip::Data to_data     = rc.getOriginalRequest().header(h_To).uri().getAor(); 
    resip::Data from_data   = rc.getOriginalRequest().header(h_From).uri().getAor(); 
    from_data += to_data;

    std::shared_ptr<NotifyToInviteChannel> handleNotify( new NotifyToInviteChannel(rc, this) );
    bool bRet = proxyHandle->SetNotifyInviteHandle(from_data, handleNotify);
    if (bRet == false)
    {
        ErrLog ( << "SetNotifyInviteHandle() fail, need to del and add new one, fromto: " << from_data );
        proxyHandle->DelNotifyInviteHandle(from_data);
        proxyHandle->SetNotifyInviteHandle(from_data, handleNotify);
    }

    rc.m_NotifyStatus = Sent_PushMsg_Status;
    DebugLog ( << "update rc status => after push msg status: " << Sent_PushMsg_Status 
              << ", tid: " << rc.getTransactionId() 
              << ", fromto: " << from_data);
    return true;
}

bool PushIotServer::PostWaitNotifymMsgTimer(RequestContext& rc, Data& dCallId )
{
    NotifyTimerMsg* pTimerMsg = new NotifyTimerMsg( *this, rc.getTransactionId(),&rc.getProxy() );
    pTimerMsg->m_CallId = dCallId;

    Proxy* proxy = &(rc.getProxy());
    
    int ms_wait_notify_tm = m_ProxyConfig.getConfigInt("WaitNotifyTm",100); //default is: 100 ms
    DebugLog( << "post timer msg, tm_ms: " << ms_wait_notify_tm << ", tid: "
             << rc.getTransactionId() <<", callid: " << pTimerMsg->m_CallId);

    proxy->postMS(std::auto_ptr<resip::ApplicationMessage>(pTimerMsg), ms_wait_notify_tm);
    return true;
}

bool PushIotServer::MakeTargetNodes(Message *asyc_msg, RequestContext& rc)
{
    NotifyAsyncMesg* async = dynamic_cast<NotifyAsyncMesg*>(asyc_msg);
    if ( !async  )
    {
        return false;
    }

    resip::ContactList& vContext = async->m_vdstContact;
    if ( vContext.size() > 0 )
    {
        TargetPtrList batch;
        for (resip::ContactList::iterator iter = vContext.begin(); iter != vContext.end(); ++iter)
        {
             resip::ContactInstanceRecord contact = *iter;
             QValueTarget* target = new QValueTarget( contact );
             batch.push_back(target);
        }
        
        if (!batch.empty())
        {
#ifdef __SUNPRO_CC
            sort(batch.begin(), batch.end(), Target::priorityMetricCompare);
#else
            batch.sort(Target::priorityMetricCompare); 
#endif 
            rc.getResponseContext().addTargetBatch(batch, false);
            return true;
        }
    }

    WarningLog( << "notify send contacts num: " <<  vContext.size() );
    return false;
}

bool PushIotServer::SetMtsPortWithCallId(repro::RequestContext& rc, PushIotServerAsyncMessage& asyn_msg)
{
    Proxy* pProxyHandle = &(rc.getProxy());
    MediaPortSource* pPorts = new MediaPortSource(asyn_msg.mMediaPortData);
    resip::SipMessage& reqMsg =  rc.getOriginalRequest();
    const resip::Tuple& receivedTransportTuple = reqMsg.getReceivedTransportTuple();
    if ( resip::Tuple::toDataLower(receivedTransportTuple.getType()) == "tcp" )
    {
        DebugLog( << "recv msg on tcp transport, callid: " << rc.getOriginalRequest().header(h_CallID).value() );
        pPorts->m_CallerTcpTransport = true;
        return pProxyHandle->SetMtsRelayPorts(asyn_msg.mCall_ID, pPorts);
    }

    if ( reqMsg.header(h_RequestLine).uri().exists(p_transport) == false )
    {
        DebugLog( << "has not transport field in request-uri, callid: " << rc.getOriginalRequest().header(h_CallID).value() );
        return pProxyHandle->SetMtsRelayPorts(asyn_msg.mCall_ID, pPorts);
    }

    resip::TransportType type  = toTransportType(reqMsg.header(h_RequestLine).uri().param(p_transport)); 
    if (type == resip::TCP)
    {
        DebugLog (<< "call id: " << rc.getOriginalRequest().header(h_CallID).value() <<",transport=tcp");
        pPorts->m_CallerTcpTransport = true;
    }
    
    return pProxyHandle->SetMtsRelayPorts(asyn_msg.mCall_ID, pPorts);
}


bool PushIotServer::PostReleaseMtsPortMsg( RequestContext& rc, const std::string& sIp, unsigned int uiPort )
{
    ReleaseMtsPortAsyncMsg* asyncReleaseMsg = new ReleaseMtsPortAsyncMsg(*this, 
                                                                         rc.getTransactionId(),
                                                                         &rc.getProxy());
    asyncReleaseMsg->m_MtsIp    = sIp;
    asyncReleaseMsg->m_MtsPort  = uiPort;
    asyncReleaseMsg->m_CallId   =  rc.getOriginalRequest().header(h_CallID).value();
    
    DebugLog( << "post async release mts port msg ...." );
    std::auto_ptr<ApplicationMessage> shrAppMsg(asyncReleaseMsg);
    mAsyncDispatcher->post(shrAppMsg);
    return true;
}


bool PushIotServer::ReleaseRemoteMediaPort(const resip::Data& dCallId,
                                           const std::string& sIp, 
                                           unsigned int iPort)
{
    std::auto_ptr<ConServerSynClient> MediaRealsePort(new ConServerSynClient(sIp, iPort));
    if (false == MediaRealsePort->ConnectServer())
    {
        ErrLog(<<"connect media server fail, mts ip: " << sIp <<", port: " << iPort);
        return false;
    }

    rapidjson::Document oroot;
    oroot.SetObject();
    rapidjson::Document::AllocatorType &allocator = oroot.GetAllocator();

    std::string method = CMD_NO_RELEASE_RELAY_PORT_REQ;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int timestamp = tv.tv_sec;

    std::string traceId(dCallId.c_str());
    std::string spanId  = "88";

    rapidjson::Value method_;
    method_.SetString( method.c_str() , method.length() , allocator ) ;
    oroot.AddMember("method", method_, allocator);

    oroot.AddMember("timestamp", timestamp, allocator);
    oroot.AddMember("req_id", timestamp, allocator);

    rapidjson::Value traceid_;
    traceid_.SetString( traceId.c_str() , traceId.length() , allocator ) ;
    oroot.AddMember("traceId", traceid_, allocator);

    rapidjson::Value spanid_;
    spanid_.SetString( spanId.c_str() , spanId.length() , allocator ) ;
    oroot.AddMember("spanId", spanid_, allocator);

    rapidjson::Document params;
    params.SetObject();
    rapidjson::Document::AllocatorType &sallocator = params.GetAllocator();

    rapidjson::Value params_;
    params_.SetString( dCallId.c_str(), dCallId.size() , sallocator ) ;
    params.AddMember("session_id", params_, sallocator);//获取会话ID为callid
    oroot.AddMember("params", params, sallocator);

    InfoLog(<< "start Send req => Release port to Media Server....");
    if (false == MediaRealsePort->DoWrite(oroot))
    {
        ErrLog( << "send req release port fail, callid: " << dCallId);
        return false;
    }

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    oroot.Accept(writer);
    std::string sResultData = buffer.GetString();
    DebugLog(<< "req release media Server data = " << sResultData);

    rapidjson::Document rspJsonData;
    if ( MediaRealsePort->DoRead(rspJsonData) == false )
    {
        ErrLog ( << "recv release mts port response fail, callid: " << dCallId );
        return false;
    }

    if ( rspJsonData.HasParseError() )
    {
        ErrLog ( << "parse release mts port response fail, callid: " << dCallId );
        return false;
    }

    if ( !rspJsonData.IsObject() )
    {
        ErrLog ( << "release mts port response not object, callid: " << dCallId );
        return false;
    }

    if ( rspJsonData.HasMember("code") && rspJsonData["code"].IsInt() )
    {
        if (rspJsonData["code"].GetInt() != 0)
        {
            ErrLog( << "release mts relay port fail," 
                   << "callid: " << dCallId << ", err msg: " 
                   << rspJsonData["msg"].GetString() );
            return false;
        }
    }
    return true;
}

bool PushIotServer::ModifyContactByReqViaRport( RequestContext& rc )
{
    resip::SipMessage& reqSipMsg = rc.getOriginalRequest();
    if ( reqSipMsg.exists(h_Vias) == false  || reqSipMsg.header(h_Vias).empty() )
    {
        ErrLog( << "req msg Vias is empty" );
        return false;
    }

    int iRPortData = 0;

    for ( Vias::const_iterator viaIt = reqSipMsg.header(h_Vias).begin();
         viaIt != reqSipMsg.header(h_Vias).end(); viaIt++ )
    {
        if (viaIt->isWellFormed() == false)
        {
            continue;
        }
        DebugLog (<< "Vias: " << *viaIt <<", exist: " << viaIt->exists(p_rport) << ", hasvalue(): " 
                  << viaIt->param(p_rport).hasValue() <<",value: " << viaIt->param(p_rport).port());

        if ( viaIt->exists(p_rport) == false )
        {
            continue;
        }

        iRPortData = viaIt->param(p_rport).port();
        break;
    }

    if (iRPortData <= 0 )
    {
        ErrLog( << "not get rport field in vias" );
        return false;
    }

    ParserContainer<NameAddr>& contactList = reqSipMsg.header(h_Contacts);
    ParserContainer<NameAddr>::iterator itContact = contactList.begin();
    for ( ; itContact != contactList.end(); ++itContact)
    {
        DebugLog( << "orig [ ip:port ] => [ " << itContact->uri().host() <<":" 
                 <<  itContact->uri().port() << " ]" << ", new [ ip:port ] => [ "
                 << Tuple::inet_ntop( reqSipMsg.getSource() ) << ":" << iRPortData );
        itContact->uri().port() = iRPortData;
        itContact->uri().host() = Tuple::inet_ntop( reqSipMsg.getSource() );
    }
    return true;
}
