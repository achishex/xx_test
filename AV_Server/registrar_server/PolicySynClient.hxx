#if !defined(RESIP_POLICYSYNCLIENT_HXX)
#define RESIP_POLICYSYNCLIENT_HXX

#include "t_socket.hxx"
#include "t_proto.hxx"

namespace repro
{
    class PolicySynClient: public Sock
    {
     public:
      PolicySynClient(const std::string &sIp, int iPort);
      virtual ~PolicySynClient();

	  bool ConnectPolicyServer();
	  
   
      bool DoRead(rapidjson::Document &jsonData);
      bool DoWrite(rapidjson::Document &jsonData);
      std::string GetKeyItemVal(const std::string& sKey, rapidjson::Document &root);
	  
     private:
      CBuffer* pRecvBuff;   //接收缓冲区
      CBuffer* pSendBuff;   //发送缓冲区
      BusiCodec* m_pCodec;

    };
}
#endif
