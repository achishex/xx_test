#if !defined(RESIP_RESPRELEASEMTSPORT_HXX)
#define RESIP_RESPRELEASEMTSPORT_HXX

#include "rutil/Data.hxx"
#include "AsyncProcessor.hxx"
#include "ProxyConfig.hxx"

namespace resip
{
    class SipStack;
}

namespace repro
{


class RespReleaseMtsPort: public AsyncProcessor
{
 public:
  RespReleaseMtsPort( ProxyConfig& config, Dispatcher* asyncDispatcher );
  virtual ~RespReleaseMtsPort();
  
  virtual processor_action_t process(RequestContext &);
  virtual bool asyncProcess(AsyncProcessorMessage* msg);
 private:
  short parseActionResult(const resip::Data& result, resip::Data& rejectReason);
  processor_action_t applyActionResult(RequestContext &rc, const resip::Data& actionResult);

  bool ReleaseRemoteMediaPort( const resip::Data& sCallId, const std::string& sIp,
                              short int iPort);
  resip::Data m_DefaultErrBehavior;
};

}
#endif
