#include <iostream>
#include "ConServerSynClient.hxx"
#include "rutil/Logger.hxx"

#include <errno.h>
#include <string>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO


namespace repro
{
    ConServerSynClient::ConServerSynClient(const std::string &sIp, int iPort)
        :Sock(sIp,iPort),m_pCodec(NULL)
    {
        m_pCodec = new BusiCodec(1);
	
        pRecvBuff = new CBuffer();
        pSendBuff = new CBuffer();
    }

    ConServerSynClient::~ConServerSynClient()
    {
        if (pRecvBuff)
        {
            delete pRecvBuff; 
            pRecvBuff = NULL;
        }
        //
        if (pSendBuff)
        {
            delete pSendBuff; 
            pSendBuff = NULL;
        }

        if(m_pCodec)
        {
            delete m_pCodec;
        }
    }

    bool ConServerSynClient::ConnectServer()
    {
        return Connect();
    }
	
    bool ConServerSynClient::DoRead(rapidjson::Document &jsonData)
    {
        //PL_LOG_DEBUG("accept fd: %d begin to recv data", m_iFd);
        DebugLog(<< "accept fd: " << m_iFd << " begin to recv data");
        while(1)
        {
	        int iErrNo = 0;
	        pRecvBuff->Compact(8*1024);
	        int iRet =  pRecvBuff->ReadFD(m_iFd, iErrNo);
	        
			
	        if (iRet < 0)
	        {
	            if (EINTR != iErrNo && EAGAIN != iErrNo)
	            {
					ErrLog(<< "recv err from client on fd: " << m_iFd << ", errmsg: " << strerror(iErrNo));
	                return false;
	            }
	            continue;
	        }

	        if (iRet == 0)
	        {
				InfoLog(<< "peer client active close socket fd: " << m_iFd);
	            return true;
	        }

			DebugLog(<< "recv from fd: " << m_iFd << " data len: " << iRet <<", ReadableBytes() = "<< pRecvBuff->ReadableBytes() 
			         << ", read_index: " <<pRecvBuff->GetReadIndex());
	        //rapidjson::Document jsonData, jsonDataResp;

	        CODERET codeRet = m_pCodec->DeCode(pRecvBuff, jsonData);
	        if ( codeRet == RET_RECV_NOT_COMPLETE )
	        {
	            //PL_LOG_DEBUG("not complete recv data on fd: %d", m_iFd);
	            DebugLog(<< "not complete recv data on fd: " << m_iFd);
	            continue;
	        }
	        else if ( codeRet == RET_RECV_OK )
	        {
	            break;	
	        }
	        else if ( codeRet == RET_RECV_ERR )
	        {
	            //PL_LOG_ERROR("parse recv data fail, on fd: %d", m_iFd);
	            ErrLog(<< "parse recv data fail, on fd: " << m_iFd);
	            return true;
	        }
	        else 
	        {
                return true;
	        }        
        }

        return true;
    }

    bool ConServerSynClient::DoWrite(rapidjson::Document &jsonData)
    {

	    //jsonDataResp is json format
	    CODERET retEncode = m_pCodec->EnCode(pSendBuff, jsonData);
	    if (retEncode != RET_RECV_OK)
	    {
	        return false;
	    }
	
        int iErrNo = 0;

        int iNeedWriteLen = pSendBuff->ReadableBytes();

        int iAlreadyWriteLen = 0;

		while(1)
		{
	        
	        int iWriteLen = pSendBuff->WriteFD(m_iFd, iErrNo);
	        pSendBuff->Compact(8*1024);
	        
	        if (iWriteLen < 0)
	        {
	            if (EAGAIN != iErrNo && EINTR != iErrNo)
	            {
	                ErrLog(<< "send to fd: " << m_iFd << ", err msg: " << strerror(iErrNo));
	                return false;
	            }

	            if (EAGAIN == iErrNo)
	            {
	                //StackLog(<< "interrupt, send to fd go on, fd: " << m_iFd);
	                continue;
	            }
	            else
	            {
	                DebugLog(<< "send to fd interrupt, fd: " << m_iFd);
	                continue;
	            }
	        }
	        else if (iWriteLen > 0)
	        {
	            iAlreadyWriteLen += iWriteLen;
				
	            if (iAlreadyWriteLen == iNeedWriteLen)
	            {
	                DebugLog(<< "send data len: " << iWriteLen << " to peer ok on fd: " << m_iFd);
	                break;
	            }
	            else 
	            {
	                //PL_LOG_DEBUG("has write data len: %d, to write data len: %d, on fd: %d",
	                //             iWriteLen, iNeedWriteLen, m_iFd); 
	                DebugLog(<< "has write data len: " << iWriteLen << ", to write data len: " << iNeedWriteLen 
	                         << ", on fd: " << m_iFd);
	            }
	        }
	        else 
	        {
	            continue;
	        }		
		}

        return true;
    }


    std::string ConServerSynClient::GetKeyItemVal(const std::string& sKey, rapidjson::Document &root)
    {
        if (root.IsObject() == false)
        {
            //PL_LOG_ERROR("json root not obj ");
            return std::string("");
        }
        if ( !root.HasMember(sKey.c_str()) || !root[sKey.c_str()].IsString() )
        {
            //PL_LOG_ERROR("json root has not field: %s, or field: %s val not string format", 
            //             sKey.c_str(), sKey.c_str());
            return std::string("");
        }
        return root[sKey.c_str()].GetString();
    }

}


