#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"

#include "AsyncProcessorMessage.hxx"
#include "monkeys/RedirectServer.hxx"
#include "RequestContext.hxx"
#include "Proxy.hxx"

#include "resip/stack/SipStack.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "resip/stack/ExtensionHeader.hxx"

#include "protocol/proto_inner.h"

#include <sys/time.h>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;
using namespace rapidjson;

class RedirectServerAsyncMessage : public AsyncProcessorMessage
{
public:
	RedirectServerAsyncMessage(AsyncProcessor& proc,
		const resip::Data& tid,
		TransactionUser* passedtu) :
		AsyncProcessorMessage(proc, tid, passedtu),
		mQueryResult(0)
	{
	}

	Data mQueryResultData;
	NameAddr mSipServerData;
	int mQueryResult;
};

RedirectServer::RedirectServer(ProxyConfig& config,
	Dispatcher* asyncDispatcher,
	resip::RegistrationPersistenceManager& store) :
	AsyncProcessor("RedirectServer", asyncDispatcher),
	mDefaultErrorBehavior("500, Server Internal DB Error"),
	mSipPolicyIP(config.getConfigData("SipPolicyIP", "10.101.70.52")),
    mSipPolicyPort(config.getConfigInt("SipPolicyPort", 7878)),
    mStore(store)
    {
}

RedirectServer::~RedirectServer()
{
}

short
RedirectServer::parseActionResult(const Data& result, Data& redirectReason)
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
RedirectServer::applyActionResult(RequestContext &rc, const Data& actionResult, const resip::NameAddr& serverAddr)
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

            Data inputUriIndex = inputUri.toString();
			Data refixSip("sip:");
            Data refixP2P("P2P_");
			Data refixAll("sip:P2P_");
			Data inputUriIndexParsed;

            InfoLog(<< "Get URI: " << inputUriIndex );
			
            if(inputUriIndex.prefix(refixAll))
            {
                //如果请求的是p2p的请求，则需要把前缀去掉
                inputUriIndexParsed = refixSip;
                inputUriIndexParsed += inputUriIndex.substr(refixAll.size(), inputUriIndex.size() - refixAll.size());
                inputUriIndex = inputUriIndexParsed;

				InfoLog(<< "Parse URI: " << inputUriIndex << " with p2p type ");
            }

	        resip::Uri inputUriKey(inputUriIndex);

            InfoLog(<< "Get contact: " << inputUriKey << " from contacts ");
			
		    resip::ContactList contacts;
		    mStore.getContacts(inputUriKey,contacts);			

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
RedirectServer::process(RequestContext &rc)
{
	DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << rc);

	Message *message = rc.getCurrentEvent();

	RedirectServerAsyncMessage *async = dynamic_cast<RedirectServerAsyncMessage*>(message);

	if (async)
	{
		if (async->mQueryResult == 0)  // If query was successful, then get query result
		{
			InfoLog(<< "get sipserver completed successfully: Result=" << async->mQueryResult << ", resultData=" << async->mQueryResultData);
			Data userName = rc.getOriginalRequest().header(h_To).uri().user();
			async->mSipServerData.uri().user() = userName;
			return applyActionResult(rc, async->mQueryResultData, async->mSipServerData);
		}
		else
		{
			InfoLog(<< "get sipserver failed: Result=" << async->mQueryResult);

			return applyActionResult(rc, mDefaultErrorBehavior, async->mSipServerData);
		}
	}
	else
	{
		// Dispatch async
		std::auto_ptr<ApplicationMessage> async(new RedirectServerAsyncMessage(*this, rc.getTransactionId(), &rc.getProxy()));
		mAsyncDispatcher->post(async);
		return WaitingForEvent;
	}
}

bool
RedirectServer::asyncProcess(AsyncProcessorMessage* msg)
{
	RedirectServerAsyncMessage* async = dynamic_cast<RedirectServerAsyncMessage*>(msg);
	resip_assert(async);

	async->mQueryResultData = Data("302, Moved Temporarily");
	async->mSipServerData.uri().scheme() = "sip";
	async->mSipServerData.uri().user() = Data("");
	async->mSipServerData.uri().host() = Data("10.101.70.53");
	async->mQueryResult = 0;
    
	//Sleep(2000);
	
	//增加向policyserver请求代理服务器ip+port的动作
	std::auto_ptr<PolicySynClient> plySynClient(new PolicySynClient(mSipPolicyIP.c_str(),mSipPolicyPort));

	if(!plySynClient->ConnectPolicyServer())
	{
	    ErrLog(<< "Connect Policy Server Failed");
	}
		rapidjson::Document reqJsonData;
		rapidjson::Document rspJsonData;

	    buildRequestSipProxy(reqJsonData);

	    string resultData = "";

        InfoLog(<< "start IO....");
		
		plySynClient->DoWrite(reqJsonData);

	    resultData = ConverJason2String(reqJsonData);

	    InfoLog(<< "req data =" << resultData);
		
	    plySynClient->DoRead(rspJsonData);
	    
	    resultData = ConverJason2String(rspJsonData);

		InfoLog(<< "rsp data =" << resultData);

		int port = 5060;
		
		std::string strSipServerIP = getSipProxyIP(rspJsonData , port);

        InfoLog(<< "sip server ip =" << strSipServerIP << " port ="<< port);

		async->mSipServerData.uri().host() = Data(strSipServerIP.c_str());	
	
	    async->mSipServerData.uri().port() = port;
		
	return true;
}

void 
RedirectServer::buildRequestSipProxy(rapidjson::Document &oroot)
{
	oroot.SetObject();
	rapidjson::Document::AllocatorType &allocator = oroot.GetAllocator();

    std::string method = CMD_NO_GET_SIP_REQ;

	//time_t t = time(0);
    //char ch[64];
    //strftime(ch, sizeof(ch), "%Y-%m-%d %H-%M-%S", localtime(&t));
    //string timestamp = ch;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    int timestamp = tv.tv_sec;


    //int reqID = 123;
	std::string traceId = "123";
	std::string spanId  = "123";

	rapidjson::Value method_;
    method_.SetString( method.c_str() , method.length() , allocator ) ;
    oroot.AddMember("method", method_, allocator);

	//rapidjson::Value timestamp_;
    //timestamp_.SetString( timestamp.c_str() , timestamp.length() , allocator ) ;
    //oroot.AddMember("timestamp", timestamp_, allocator);

    oroot.AddMember("timestamp", timestamp, allocator);
	
	oroot.AddMember("req_id", timestamp, allocator);

	rapidjson::Value traceid_;
    traceid_.SetString( traceId.c_str() , traceId.length() , allocator ) ;
    oroot.AddMember("traceId", traceid_, allocator);

	rapidjson::Value spanid_;
    spanid_.SetString( spanId.c_str() , spanId.length() , allocator ) ;
    oroot.AddMember("spanId", spanid_, allocator);
	
}

std::string 
RedirectServer::getSipProxyIP(rapidjson::Document &inroot, int &port)
{
    std::string sipProxyIP = "";
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
        if ( inroot.HasMember("result") && inroot["result"].IsObject() )
        {
           rapidjson::Value info_object(rapidjson::kObjectType);
           info_object.SetObject();
           info_object = inroot["result"].GetObject();
		   if(info_object.HasMember("ip")&&info_object["ip"].IsString())
		   {
		       sipProxyIP = info_object["ip"].GetString();
		   }
		   if(info_object.HasMember("port")&&info_object["port"].IsInt())
		   {
		       port = info_object["port"].GetInt();
		   }		   
        }		
	}while(0);

    return sipProxyIP;
}

std::string
RedirectServer::ConverJason2String(rapidjson::Document &inroot)
{
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    inroot.Accept(writer);
	
    std::string sResultData = buffer.GetString();
    return sResultData;

}

