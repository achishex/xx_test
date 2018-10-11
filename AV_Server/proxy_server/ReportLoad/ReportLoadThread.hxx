#if !defined(RESIP_LOADREPORT_THREAD_HXX)
#define RESIP_LOADREPORT_THREAD_HXX

#include <memory>
#include "rutil/ThreadIf.hxx"
#include "ReportLoadWork.hxx"

namespace repro
{

class RL_WorkThread: public  resip::ThreadIf
{
 public:
  RL_WorkThread(std::shared_ptr<RL_Work> pRlWork);
  virtual ~RL_WorkThread();
  void thread();

 protected:
  std::shared_ptr<RL_Work>  m_pRlWork;
};

}

#endif
