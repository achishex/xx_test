#ifndef _T_PORT_POOL_H_
#define _T_PORT_POOL_H_

#include <mutex>
#include <list>
#include <map>
#include <string>
#include <mutex>

#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <queue>
#include <iostream>
#include <utility>
#include <vector>
#include <sstream>

struct RelayPortRecord
{
    //just think sip call-ID max len is 128 bytes
    char m_sSessId[128];

    unsigned short usCallerARtpPort;
    unsigned short usCallerARtcpPort;
    unsigned short usCallerVRtpPort;
    unsigned short usCallerVRtcpPort;
    //
    unsigned short usCalleeARtpPort;
    unsigned short usCalleeARtcpPort;
    unsigned short usCalleeVRtpPort;
    unsigned short usCalleeVRtcpPort;

    //
    unsigned short usRelayCmd;

    RelayPortRecord(): usCallerARtpPort(0),
        usCallerARtcpPort(0),usCallerVRtpPort(0),usCallerVRtcpPort(0),
        usCalleeARtpPort(0),usCalleeARtcpPort(0),
        usCalleeVRtpPort(0),usCalleeVRtcpPort(0), 
        usRelayCmd(0)
    {
        ::memset(m_sSessId,0,sizeof(m_sSessId));
    }

    RelayPortRecord(const RelayPortRecord& item)
    {
        ::memcpy( m_sSessId, item.m_sSessId, sizeof(m_sSessId) );

        usCallerARtpPort    = item.usCallerARtpPort;
        usCallerARtcpPort   = item.usCallerARtcpPort;;
        usCallerVRtpPort    = item.usCallerVRtpPort;
        usCallerVRtcpPort   = item.usCallerVRtcpPort;
        //
        usCalleeARtpPort    = item.usCalleeARtpPort;
        usCalleeARtcpPort   = item.usCalleeARtcpPort;
        usCalleeVRtpPort    = item.usCalleeVRtpPort;
        usCalleeVRtcpPort   = item.usCalleeVRtcpPort;
        usRelayCmd          = item.usRelayCmd;
    }

    RelayPortRecord & operator = (RelayPortRecord& item)
    {
        if (this != &item)
        {
            ::memcpy( m_sSessId, item.m_sSessId, sizeof(m_sSessId) );

            usCallerARtpPort    = item.usCallerARtpPort;
            usCallerARtcpPort   = item.usCallerARtcpPort;;
            usCallerVRtpPort    = item.usCallerVRtpPort;
            usCallerVRtcpPort   = item.usCallerVRtcpPort;
            //
            usCalleeARtpPort    = item.usCalleeARtpPort;
            usCalleeARtcpPort   = item.usCalleeARtcpPort;
            usCalleeVRtpPort    = item.usCalleeVRtpPort;
            usCalleeVRtcpPort   = item.usCalleeVRtcpPort;
            usRelayCmd          = item.usRelayCmd;
        }
        return *this;
    }
    std::string ToString()
    {
        std::stringstream ios;
        ios << "[sessionid, "
            << "caller_artp_port, caller_artcp_port, caller_vrtp_port, caller_vrtcp_port, "
            << "callee_artp_port, callee_artcp_port, callee_vrtp_port, callee_vrtcp_port, "
            << "relay_cmd]"
            << " => "
            << "[" << m_sSessId << ","
            << usCallerARtpPort << "," << usCallerARtcpPort << "," << usCallerVRtpPort << "," << usCallerVRtcpPort << ","
            << usCalleeARtpPort << "," << usCalleeARtcpPort << "," << usCalleeVRtpPort << "," << usCalleeVRtcpPort << ","
            << usRelayCmd
            << "]";
        return ios.str();
    }
};

class PortPool
{
 public:
  PortPool();
  virtual ~PortPool();

  void InitPortPool(unsigned short usUdpBase, unsigned short usUdpPortCount);
  std::pair<unsigned short, unsigned short> GetUdpRtpRtcpPort( );

  std::pair<unsigned short, unsigned short> GetAndCheckUdpRtpRtcpPort(int iRedo = 3);

  void RecycleUdpPort(std::pair<unsigned short,unsigned short>  usPort);
  unsigned int GetUsablePortNums();
  unsigned int GetPortPoolCap() const;

 public:
  std::map<std::string, RelayPortRecord> m_mpClientSession;
 private:
  bool CheckUdpPortPairsUsable(std::pair<unsigned short,unsigned short>  usPort);

  bool CheckPortUsable(unsigned short usPort);
  unsigned short m_usUdpPortBase;
  unsigned short m_usUdpPortCount;
  //
  std::mutex m_udpPoolMutex;
  typedef std::priority_queue<unsigned short, std::vector<unsigned short>,std::greater<unsigned short> > PriQueType;
  PriQueType m_pqPortPools;
};

#endif
