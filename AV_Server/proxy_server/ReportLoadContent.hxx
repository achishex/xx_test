/**
 * @file: ReportLoadContent.hxx
 * @brief: 
 * @author: achilsh 
 * @version: v0x0001
 * @date: 2018-09-20
 */

#if !defined(REPORTLOADCONTENT_HXX)
#define REPORTLOADCONTENT_HXX

#include <string>
#include "rutil/RWMutex.hxx"
#include "rutil/Lock.hxx"

namespace repro
{

class ReportLoadContent
{
 public:
  ReportLoadContent();
  virtual ~ReportLoadContent();

 public:
  struct LoadContent
  {
      LoadContent():m_sLocalHost(""),
                    m_iLocalPort(0),
                    m_iConnNums(0),
                    m_iTotalConn(0)
      {}
      ~LoadContent() 
      {
          m_sLocalHost.clear(); //负载上报的主机对外服务的ip

          m_iLocalPort  = 0;   //负载上报的主机对外服务端口
          m_iConnNums   = 0;
          m_iTotalConn  = 0;
      }

      std::string       m_sLocalHost;
      int               m_iLocalPort;
      volatile int      m_iConnNums;
      volatile int      m_iTotalConn;
  };
  
  void UpdateLoadHost(const std::string& sHost);
  void UpdateLoadPort(int iPort);
  void IncrConnNums();
  
  ReportLoadContent::LoadContent GetLoad();

 private:
  resip::RWMutex    mMutex;
  LoadContent       m_loadCont;
  int               m_iCurTm;

 private:
  ReportLoadContent(const ReportLoadContent& cont);
  ReportLoadContent& operator = (const ReportLoadContent& cont);
};

}
#endif
