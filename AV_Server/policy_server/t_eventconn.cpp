#include <iostream>
#include "t_eventconn.hpp"
#include "t_worktask.hpp"
#include "t_proto.hpp"
#include "proto_inner.h"

#include "t_busi_module.h" 
#include "t_busi_interface.h"

#include <errno.h>
#include <string>

namespace T_TCP
{

    std::map<std::string, std::shared_ptr<PolicyBusi>> AcceptConn::m_mpBusiModule = AcceptConn::BuildBusiModule(); 

    AcceptConn::AcceptConn(int ifd, void* pData)
        :ConnBase(ifd), m_pData((WorkerTask*)pData),m_pCodec(NULL)
    {
        m_pCodec = new BusiCodec(1);
        
        pRecvBuff = new CBuffer();
        pSendBuff = new CBuffer();
        
    }

    AcceptConn::~AcceptConn()
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

        if (m_pCodec)
        {
            delete m_pCodec; m_pCodec = NULL;
        }
    }

    bool AcceptConn::AddEvent(struct event_base *pEvBase, int iEvent, void *arg)
    {
        if (pEvBase == NULL)
        {
            return false;
        }

        if (EV_READ & iEvent)
        {
            event_assign(&m_stREvent, pEvBase, m_iFd, iEvent, ReadCallBack, this);
            event_add(&m_stREvent, NULL); 
        } 
        else 
        {
            event_assign(&m_stWEvent, pEvBase, m_iFd, iEvent, WriteCallBack, this);
            event_add(&m_stWEvent, NULL);
        }
        return true;
    }

    void AcceptConn::ReadCallBack(int iFd, short sEvent, void *pData)
    {
        if (iFd <= 0)
        {
            return ;
        }

        AcceptConn* pAcceptConn = (AcceptConn*)pData;
        if (pAcceptConn == NULL)
        {
            return ;
        }
        pAcceptConn->DoRead();
    }

    //
    bool AcceptConn::DoRead()
    {
        PL_LOG_DEBUG("accept fd: %d begin to recv data", m_iFd);
       
        int iErrNo = 0;
        pRecvBuff->Compact(8*1024);
        int iRet =  pRecvBuff->ReadFD(m_iFd, iErrNo);
        
        PL_LOG_DEBUG("recv from fd: %d data len: %d, ReadableBytes() = %d, read_index: %d",
                     m_iFd, iRet, pRecvBuff->ReadableBytes(), pRecvBuff->GetReadIndex());
       
        if (iRet < 0)
        {
            if (EINTR != iErrNo && EAGAIN != iErrNo)
            {
                PL_LOG_ERROR("recv err from client, errmsg: %s", strerror(iErrNo));
                m_pData->DeleteAcceptConn(m_iFd);
                return false;
            }
            return true;
        }

        if (iRet == 0)
        {
            PL_LOG_INFO("peer client active close socket fd: %d", m_iFd);
            m_pData->DeleteAcceptConn(m_iFd);
            return true;
        }

        rapidjson::Document jsonData, jsonDataResp;

        CODERET codeRet = m_pCodec->DeCode(pRecvBuff, jsonData);
        if ( codeRet == RET_RECV_NOT_COMPLETE )
        {
            PL_LOG_DEBUG("not complete recv data on fd: %d", m_iFd);
            return true;
        }
        else if ( codeRet == RET_RECV_OK )
        {
            //do busi logic:
            std::string method_val;
            do {
                method_val = GetKeyItemVal("method", jsonData);
                if (method_val.empty())
                {
                    //build resp json 
                    BuildErrResp(jsonDataResp, jsonData, -1, "method field invaild");
                    PL_LOG_ERROR("method field val is empty");
                    break;
                }
                PL_LOG_DEBUG("client req method: %s", method_val.c_str());

                std::shared_ptr<PolicyBusi> pBusiModule = GetBusiModule(method_val);
                if (!pBusiModule)
                {
                    //build resp json 
                    BuildErrResp(jsonDataResp, jsonData, -1, "server not register busi module");
                    PL_LOG_ERROR("not get busi module for method: %s", method_val.c_str());
                    break;
                }

                pBusiModule->ProcessBusi(jsonData, jsonDataResp);

            } while (0);
          
            //jsonDataResp is json format
            CODERET retEncode = m_pCodec->EnCode(pSendBuff, jsonDataResp);
            if (retEncode != RET_RECV_OK)
            {
                m_pData->DeleteAcceptConn(m_iFd);
                PL_LOG_ERROR("encode response fail");
                return false;
            }

            AddEvent(m_pData->GetEventBase(), EV_WRITE, this);
        }
        else if ( codeRet == RET_RECV_ERR )
        {
            PL_LOG_ERROR("parse recv data fail, on fd: %d", m_iFd);
            m_pData->DeleteAcceptConn(m_iFd);
        }
        else 
        {

        }
        return true;
    }

    void AcceptConn::WriteCallBack(int iFd, short sEvent, void *pData)
    {
        if (iFd <= 0)
        {
            return ;
        }
        AcceptConn* pAcceptConn = (AcceptConn*)pData;
        if (pAcceptConn == NULL)
        {
            return ;
        }
        pAcceptConn->DoWrite();
    }

    bool AcceptConn::DoWrite()
    {
        int iErrNo = 0;
        int iNeedWriteLen = pSendBuff->ReadableBytes();

        int iWriteLen = pSendBuff->WriteFD(m_iFd, iErrNo);
        pSendBuff->Compact(8*1024);
        
        if (iWriteLen < 0)
        {
            if (EAGAIN != iErrNo && EINTR != iErrNo)
            {
                PL_LOG_ERROR("send to fd: %d, err msg: %s", m_iFd, strerror(iErrNo));
                m_pData->DeleteAcceptConn(m_iFd);
                return false;
            }

            if (EAGAIN == iErrNo)
            {
                PL_LOG_DEBUG("send to fd go on, fd: %d", m_iFd);
                AddEvent(m_pData->GetEventBase(), EV_WRITE, this);
                return true;
            }
            else
            {
                PL_LOG_DEBUG("send to fd interrupt, fd: %d", m_iFd);
                AddEvent(m_pData->GetEventBase(), EV_WRITE, this);
                return true;
            }
        }
        else if (iWriteLen > 0)
        {
            if (iWriteLen == iNeedWriteLen)
            {
                PL_LOG_DEBUG("send data len: %d to peer ok on fd: %d", iWriteLen, m_iFd);
            }
            else 
            {
                PL_LOG_DEBUG("has write data len: %d, to write data len: %d, on fd: %d",
                             iWriteLen, iNeedWriteLen, m_iFd);
                AddEvent(m_pData->GetEventBase(), EV_WRITE, this);
            }
        }
        else 
        {
            return true;
        }

        return true;
    }

    bool AcceptConn::DelEvent(struct event_base *pEvBase, int iEvent, void *arg)
    {
        return true;
    }

    std::shared_ptr<PolicyBusi> AcceptConn::GetBusiModule(const std::string& sMethod)
    {
        std::shared_ptr<PolicyBusi> tmpRet;
        if (sMethod.empty())
        {
            return tmpRet; 
        }

        auto itBusiModule = AcceptConn::m_mpBusiModule.find(sMethod);
        if (itBusiModule != AcceptConn::m_mpBusiModule.end())
        {
            PL_LOG_DEBUG("find busi module by method: %s, module map size: %u",
                         sMethod.c_str(), AcceptConn::m_mpBusiModule.size());
            return itBusiModule->second;
        }
        else 
        {
            PL_LOG_ERROR("not find method process obj, method: %s", sMethod.c_str());
            return tmpRet; 
        }
    }

    std::map<std::string, std::shared_ptr<PolicyBusi>> AcceptConn::BuildBusiModule()
    {
        std::map<std::string, std::shared_ptr<PolicyBusi>>  tmp_m_mpBusiModule;
        tmp_m_mpBusiModule.clear();
        
        tmp_m_mpBusiModule[CMD_NO_REGISTER_SIP_REQ] = 
            std::make_shared<RegisterInterface>(RegisterInterface(CMD_NO_REGISTER_SIP_REQ));
        
        tmp_m_mpBusiModule[CMD_NO_REPORT_SIP_REQ] = 
            std::make_shared<RegisterInterface>(RegisterInterface(CMD_NO_REPORT_SIP_REQ));
        
        tmp_m_mpBusiModule[CMD_NO_GET_SIP_REQ] 
            = std::make_shared<NodeAllocate>(NodeAllocate(CMD_NO_GET_SIP_REQ));
        
        tmp_m_mpBusiModule[CMD_NO_REGISTER_MTS_REQ]
            = std::make_shared<RegisterInterface>(RegisterInterface(CMD_NO_REGISTER_MTS_REQ));
        
        tmp_m_mpBusiModule[CMD_NO_REPORT_MTS_REQ]
            = std::make_shared<RegisterInterface>(RegisterInterface(CMD_NO_REPORT_MTS_REQ));
        
        tmp_m_mpBusiModule[CMD_NO_GET_MTS_REQ] 
            = std::make_shared<NodeAllocate>(NodeAllocate(CMD_NO_GET_MTS_REQ));

        PL_LOG_DEBUG("register busi process module succ, module nums: %d", tmp_m_mpBusiModule.size());
        return tmp_m_mpBusiModule;
    }

    std::string AcceptConn::GetKeyItemVal(const std::string& sKey, rapidjson::Document &root)
    {
        if (root.IsObject() == false)
        {
            PL_LOG_ERROR("json root not obj ");
            return std::string("");
        }
        if ( !root.HasMember(sKey.c_str()) || !root[sKey.c_str()].IsString() )
        {
            PL_LOG_ERROR("json root has not field: %s, or field: %s val not string format", 
                         sKey.c_str(), sKey.c_str());
            return std::string("");
        }
        return root[sKey.c_str()].GetString();
    }

    void AcceptConn::BuildErrResp(rapidjson::Document& oroot, rapidjson::Document& inroot, 
                                  int iCode, const std::string& sErrMsg)
    {
        std::string sMethod("");
        int iReqId = 0; 
        std::string sTraceId("");
        std::string sSpanId("");

        int nowTime = ::time(NULL);
        std::string serrMsg;

        int code = 0;
        do {

            if ( inroot.HasParseError() )
            {
                PL_LOG_ERROR("req json parse err");
                serrMsg.assign("req json parse err");
                iCode = code = -1;
                break;
            }
            if ( !inroot.IsObject() )
            {
                PL_LOG_ERROR("req json is not obj");
                serrMsg.assign("req json is not obj");
                iCode = code = -1;
                break;
            }
            
            if ( inroot.HasMember("method") && inroot["method"].IsString() )
            {
                sMethod = inroot["method"].GetString();
            }

            if ( inroot.HasMember("req_id") && inroot["req_id"].IsInt() )
            {
                iReqId = inroot["req_id"].GetInt();
            }
            if ( inroot.HasMember("traceId") && inroot["traceId"].IsString() )
            {
                sTraceId = inroot["traceId"].GetString();
            }
            if ( inroot.HasMember("spanId") && inroot["spanId"].IsString() )
            {
                sSpanId = inroot["spanId"].GetString();
            }

            serrMsg = sErrMsg;
        } while (0);
        
        oroot.SetObject();
        rapidjson::Document::AllocatorType &allocator = oroot.GetAllocator();
        
        rapidjson::Value author;
        author.SetString(sMethod.c_str(), sMethod.size(), allocator);
        oroot.AddMember("method", author, allocator);
       
        oroot.AddMember("req_id", iReqId, allocator);
        
        author.SetString(sTraceId.c_str(), sTraceId.size(), allocator);
        oroot.AddMember("traceId", author, allocator);

        author.SetString(sSpanId.c_str(), sSpanId.size(), allocator);
        oroot.AddMember("spanId", author, allocator);

        oroot.AddMember("code", iCode, allocator);

        author.SetString(serrMsg.c_str(), serrMsg.size(), allocator);
        oroot.AddMember("msg", author, allocator);

        oroot.AddMember("timestamp", nowTime, allocator);
    }
}


