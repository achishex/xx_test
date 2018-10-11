
#ifndef _Timer_report_h_
#define _Timer_report_h_

#include <event2/event.h>

class TimerReport
{
 private:
  struct event    *m_event;
  struct timeval  *m_timeout;

 public:
  typedef void ( * CallBack  )(int fd,  short event, void * arg );
  TimerReport( struct event_base * base,  int seconds,  CallBack func,  void * arg  );
  virtual ~TimerReport();
};

#endif
