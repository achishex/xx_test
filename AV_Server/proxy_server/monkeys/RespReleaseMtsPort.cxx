#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"

#include "AsyncProcessorMessage.hxx"
#include "RequestContext.hxx"
#include "Proxy.hxx"

#include "resip/stack/SipStack.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "resip/stack/ExtensionHeader.hxx"

#include "protocol/proto_inner.h"
#include "RespReleaseMtsPort.hxx"
#include "ConServerSynClient.hxx"
#include "Processor.hxx"
#include "NotifyPrivateMesage.hxx"

#include <sys/time.h>
#include <map>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;
using namespace rapidjson;

class ReleaseMtsPortAsyncMsgBye: public AsyncProcessorMessage
{
 public:
  ReleaseMtsPortAsyncMsgBye(AsyncProcessor& proc,
                         const resip::Data& tid,
                         TransactionUser* passedtu,
                         const resip::Data& callId)
      : AsyncProcessorMessage(proc, tid, passedtu),
      mQueryResult(0), mCallId(callId), pMedia(NULL)
    { }
  virtual ~ReleaseMtsPortAsyncMsgBye()
  { }
  Proxy* GetProxyHandle()
  {
      return dynamic_cast<Proxy*>(mTu);
  }

  int mQueryResult;
  resip::Data mCallId;
  MediaPortSource* pMedia;
};


//--------------------------------------------------------------------------------------//
RespReleaseMtsPort::RespReleaseMtsPort( ProxyConfig& config, Dispatcher* asyncDispatcher )
    :AsyncProcessor("RespReleaseMtsPort", asyncDispatcher),
    m_DefaultErrBehavior("rspReleaseMtsPort")
{
}

RespReleaseMtsPort::~RespReleaseMtsPort()
{
}

Processor::processor_action_t RespReleaseMtsPort::process(RequestContext &rc)
{
    DebugLog(<< "Lemurs handling response: " << *this << "; reqcontext = " << rc);
    resip::Message* msg = rc.getCurrentEvent();
    
    resip::SipMessage* sip_msg = dynamic_cast<resip::SipMessage*>(msg);
    ReleaseMtsPortAsyncMsgBye* async_msg_first_step = dynamic_cast<ReleaseMtsPortAsyncMsgBye*>(msg);
    if ( async_msg_first_step )
    {
        //rc.getProxy().DelMtsRelayPorts(async_msg_first_step->mCallId);
        //DebugLog(<< "del mts relay ports by callid: " << async_msg_first_step->mCallId);

        if( async_msg_first_step->mQueryResult == 0 )
        {
        }
        else
        {
            ErrLog( << "release mts port fail, err no: " << async_msg_first_step->mQueryResult );
            return Processor::Continue;
        }
        return Processor::Continue;
    } 
    else if (sip_msg)
    {
        if (sip_msg->isResponse())
        {
            resip::SipMessage& originalRequest = rc.getOriginalRequest();
            if (originalRequest.method()==BYE 
                && sip_msg->header(resip::h_StatusLine).responseCode()== 200)
            {
                DebugLog( <<"post release mts port aysnc msg, tid: " << rc.getTransactionId()
                         << ", callid: " << sip_msg->header(h_CallID).value() );
                ReleaseMtsPortAsyncMsgBye* async_msg = new ReleaseMtsPortAsyncMsgBye(*this, 
                                                                               rc.getTransactionId(),
                                                                               &rc.getProxy(),
                                                                               sip_msg->header(h_CallID).value());
                DebugLog( <<"post release mts port aysnc msg, tid: " << rc.getTransactionId()
                         << ", callid: " << async_msg->mCallId
                         << ",app: " << async_msg);
                async_msg->pMedia = (rc.getProxy()).GetMtsPortsByCallId(async_msg->mCallId);
                if ( async_msg->pMedia == NULL )
                {
                    ErrLog( << "get media port handle is null, callid: " << async_msg->mCallId );
                    return Processor::Continue;
                }
                
                std::auto_ptr<ApplicationMessage> async_msg_firt_step(async_msg);
                mAsyncDispatcher->post(async_msg_firt_step);
                
                //send response 200ok
                //ResponseContext& resp_context = rc.getResponseContext();
                //resp_context.processResponse(*sip_msg);

                return Processor::WaitingForEvent;
            }
        }
        else
        {
            DebugLog(<<"msg is not response");
            return Processor::Continue;
        }
    }
    return Processor::Continue;
}

bool RespReleaseMtsPort::asyncProcess(AsyncProcessorMessage* msg)
{
    ReleaseMtsPortAsyncMsgBye* async = dynamic_cast<ReleaseMtsPortAsyncMsgBye*>( msg );
    if (async == NULL)
    {
        async->mQueryResult = -1;
        ErrLog (<<"recv msg not ReleaseMtsPortAsyncMsgBye");
        return false;
    }

    MediaPortSource* pMedia = async->pMedia;
    if (pMedia == NULL)
    {
        ErrLog( <<"get media port handle is null" );
        async->mQueryResult = -3;
        return false;
    }

    resip::Data& dCallId = async->mCallId;
    bool bRet = ReleaseRemoteMediaPort(dCallId, pMedia->m_MtsIp, pMedia->m_mtsPort);
    if (bRet == false)
    {
        async->mQueryResult = -4;

        ErrLog( << "release mts port fail, callid: " << dCallId );
        return false;
    }
    Proxy* pProxyHandle = async->GetProxyHandle(); 
    if (pProxyHandle == NULL)
    {
        ErrLog( << "proxy handle is null" );
        async->mQueryResult = 0;
        return false;
    }
    pProxyHandle->DelMtsRelayPorts(async->mCallId);
    DebugLog(<< "del mts relay ports in local by callid: " << async->mCallId);
    async->mQueryResult = 0;
    return true;
}

short RespReleaseMtsPort::parseActionResult(const resip::Data& result,
                                            resip::Data& rejectReason)
{
    return 0;
}

Processor::processor_action_t RespReleaseMtsPort::applyActionResult(RequestContext &rc, 
                                                         const resip::Data& actionResult)
{
    return Continue;
}

bool RespReleaseMtsPort::ReleaseRemoteMediaPort(const resip::Data& sCallId, 
                                                const std::string& sIp,
                                                short int iPort)
{
    std::auto_ptr<ConServerSynClient> MediaRealsePort(new ConServerSynClient(sIp, iPort));
    if (false == MediaRealsePort->ConnectServer())
    {
        ErrLog(<<"connect media server fail");
        return false;
    }

    rapidjson::Document oroot;
    oroot.SetObject();
    rapidjson::Document::AllocatorType &allocator = oroot.GetAllocator();

    std::string method = CMD_NO_RELEASE_RELAY_PORT_REQ;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int timestamp = tv.tv_sec;

	std::string traceId = "888";
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
    params_.SetString( sCallId.c_str() , sCallId.size() , sallocator ) ;
    params.AddMember("session_id", params_, sallocator);//获取会话ID为callid
	oroot.AddMember("params", params, sallocator);

    InfoLog(<< "start Send req: Release port to Media Server....");
    if (false == MediaRealsePort->DoWrite(oroot))
    {
        ErrLog( << "send req release port fail, callid: " << sCallId );
        return false;
    }

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    oroot.Accept(writer);
    std::string sResultData = buffer.GetString();
    DebugLog(<< "req release media Server data =" << sResultData);

    rapidjson::Document rspJsonData;
    if ( MediaRealsePort->DoRead(rspJsonData) == false )
    {
        ErrLog ( << "recv release mts port response fail, callid: " << sCallId );
        return false;
    }

    if ( rspJsonData.HasParseError() )
    {
        ErrLog ( << "parse release mts port response fail, callid: " << sCallId );
        return false;
    }

    if ( !rspJsonData.IsObject() )
    {
        ErrLog ( << "release mts port response not object, callid: " << sCallId );
        return false;
    }

    if ( rspJsonData.HasMember("code") && rspJsonData["code"].IsInt() )
    {
        if (rspJsonData["code"].GetInt() != 0)
        {
            ErrLog( << "release mts relay port fail," 
                   << "callid: " << sCallId << ", err msg: " 
                   << rspJsonData["msg"].GetString() );
            return false;
        }
    }
    return true;

}
