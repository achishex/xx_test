#if !defined(RESIP_REDIRECTSERVER_HXX)
#define RESIP_REDIRECTSERVER_HXX 

#include "rutil/Data.hxx"
#include "AsyncProcessor.hxx"
#include "ProxyConfig.hxx"
#include "resip/dum/RegistrationPersistenceManager.hxx"

#include "PolicySynClient.hxx"


namespace resip
{
   class SipStack;
}

namespace repro
{

class RedirectServer : public AsyncProcessor
{
   public:
	   RedirectServer(ProxyConfig& config, Dispatcher* asyncDispatcher, resip::RegistrationPersistenceManager& store);
	   ~RedirectServer();

      // Processor virutal method
      virtual processor_action_t process(RequestContext &);

      // Virtual method called from WorkerThreads
      virtual bool asyncProcess(AsyncProcessorMessage* msg);
	  
      void buildRequestSipProxy(rapidjson::Document &oroot);

	  std::string getSipProxyIP(rapidjson::Document &inroot,int &port);

	  std::string ConverJason2String(rapidjson::Document &inroot);
	  
   private:
	   short parseActionResult(const resip::Data& result, resip::Data& redirectReason);
	   processor_action_t applyActionResult(RequestContext &rc, const resip::Data& actionResult, const resip::NameAddr& serverAddr);
	   resip::Data mDefaultErrorBehavior;

	   resip::Data mSipPolicyIP;
       int mSipPolicyPort;
	   resip::RegistrationPersistenceManager& mStore;
};

}
#endif

