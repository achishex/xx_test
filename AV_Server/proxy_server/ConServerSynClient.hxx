#if !defined(RESIP_ConServerSYNCLIENT_HXX)
#define RESIP_ConServerSYNCLIENT_HXX

#include "t_socket.hxx"
#include "t_proto.hxx"

namespace repro
{
    class ConServerSynClient: public Sock
    {
     public:
      ConServerSynClient(const std::string &sIp, int iPort);
      virtual ~ConServerSynClient();

	  bool ConnectServer();
	  
   
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
