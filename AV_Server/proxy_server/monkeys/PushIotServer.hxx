#if !defined(RESIP_PushIotServer_HXX)
#define RESIP_PushIotServer_HXX 

#include "rutil/Data.hxx"
#include "AsyncProcessor.hxx"
#include "ProxyConfig.hxx"
#include "resip/dum/RegistrationPersistenceManager.hxx"

#include "NotifyPrivateMesage.hxx"
#include "ConServerSynClient.hxx"

namespace resip
{
   class SipStack;
}

namespace repro
{

class PushIotServerAsyncMessage : public AsyncProcessorMessage
{
public:
	PushIotServerAsyncMessage(AsyncProcessor& proc,
		const resip::Data& tid,
		TransactionUser* passedtu) :
		AsyncProcessorMessage(proc, tid, passedtu),
		mQueryResult(0)
	{
	}

	Data mQueryResultData;
	NameAddr mSipServerData;
	int mQueryResult;
  std::string mErrorMessage;//错误信息
	Data mCall_ID;
	Data mFrom;
	Data mTo;
  Data mAuthToken;//鉴权TOKEN
  MediaPortSource mMediaPortData;
};


class PushIotServer : public AsyncProcessor
{
   public:
	   PushIotServer(ProxyConfig& config, Dispatcher* asyncDispatcher, 
                     resip::RegistrationPersistenceManager& store);
	   virtual ~PushIotServer();

      // Processor virutal method
      virtual processor_action_t process(RequestContext &);

      // Virtual method called from WorkerThreads
      virtual bool asyncProcess(AsyncProcessorMessage* msg);
      
      //interface used for notify processing
	    bool NotifyMsgPost(const resip::ContactList& vdstContact, RequestContext& rc);
      
      /*组装策略服务器获取流媒体IP的JSON*/
      bool GetMediaIP_Port(std::auto_ptr<ConServerSynClient> policyCli,
                                       std::string &strMediaIp,int & iport);
    /*组装流媒体服务器获取流媒体端口的JSON*/
      bool  GetMediaResourePort(std::auto_ptr<ConServerSynClient> mediaCli,
                                      std::string strcallId,
                                      PushIotServerAsyncMessage* pAsyncMsg);

      /*组装发送给IOT鉴权推送的JSON*/
      bool SendAuth2Iot(std::auto_ptr<ConServerSynClient> authCli,std::string strFrom,std::string strTo,std::string strAuthToken);

      /*组装发送给IOT呼叫推送的JSON*/
      void SendPush2Iot(std::auto_ptr<ConServerSynClient> inviteCli,std::string strFrom,std::string strTo);

       /*解析JSON拿到流媒体IP*/
       bool ParseMediaIP(rapidjson::Document &inroot,std::string &strMediaIp,int & iport);
       
       /*解析JSON拿到流媒体8个端口*/
       bool ParseMediaResourcePort(rapidjson::Document &inroot,MediaPortSource &AllPortData);

    /*解析JSON获取IOT的AUTH回包*/
      bool ParseIotAuthReturnPackage(rapidjson::Document &inroot);
        /*解析JSON获取IOT的PUSH回包*/
      void ParseIotPushReturnPackage(rapidjson::Document &inroot);

	  inline std::string ConverJason2String(rapidjson::Document &inroot);
   private:
      bool SetCurReqContextProcess(RequestContext& rc);
      bool PostWaitNotifymMsgTimer(RequestContext& rc, Data& dCallId);
      bool MakeTargetNodes(resip::Message *asyc_msg, RequestContext& rc);
      bool SetMtsPortWithCallId(RequestContext& rc, PushIotServerAsyncMessage& async_msg);

      bool PostReleaseMtsPortMsg( RequestContext& rc, const std::string& sIp, unsigned int uiPort );
      bool ReleaseRemoteMediaPort(const resip::Data& dCallId, const std::string& sIp, unsigned int uiPort);

      bool ModifyContactByReqViaRport( RequestContext& rc );
   private:
	   short parseActionResult(const resip::Data& result, resip::Data& redirectReason);
	   processor_action_t applyActionResult(RequestContext &rc,
                                            const resip::Data& actionResult,
                                            const resip::NameAddr& serverAddr);
	   resip::Data mDefaultErrorBehavior;
       /*pocily config*/
       resip::Data mSipConServerIP;
       int mSipConServerPort;
       
       /*media config*/
       resip::Data mMediaServerIP;
       int mMediaServerPort;
       
       /*Iot config*/
       resip::Data mIotServerIP;
       int mIotServerPort;
       int mIotAuthPort;
       int mAuthFlag;//鉴权标志位(测试过程可先不鉴权设置为0)
       std::string mAutuEncryFlag;//是否加密标志位(鉴权在IOT那边生产环境需要加密)
	   resip::RegistrationPersistenceManager& mStore;
       MediaPortSource mMediaPortData;
       
       resip::NameAddr  mLocalSipUri;
       ProxyConfig& m_ProxyConfig;
};

}
#endif
