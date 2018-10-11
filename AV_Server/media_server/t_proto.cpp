#include "t_proto.hpp"
#include <arpa/inet.h>
#include <string.h>


#include "rapidjson/writer.h"                                                                                       
#include "rapidjson/stringbuffer.h"                                                                                 
#include "rapidjson/document.h"                                                                                     
#include "rapidjson/error/en.h"                                                                                     
#include "rapidjson/prettywriter.h" 

using namespace rapidjson;

//each package divide by "\n", each package format is json
BusiCodec::BusiCodec(int iType) :m_iCodeType(iType)
{
}

BusiCodec::~BusiCodec()
{
}

CODERET BusiCodec::EnCode(CBuffer* pBuff, rapidjson::Document& root)
{
    MTS_LOG_DEBUG("%s() ReadableBytes()=%d", __FUNCTION__, pBuff->ReadableBytes());
    if (pBuff == NULL)
    {
        MTS_LOG_ERROR("data buf is null");
        return RET_RECV_ERR;
    }

    rapidjson::StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    root.Accept(writer);
    std::string sPackage = buffer.GetString();
    MTS_LOG_DEBUG("encode pkg: %s", sPackage.c_str());
    
    sPackage.append("\n");
    unsigned int iSendRet = pBuff->Write(sPackage.c_str(), sPackage.size());
    if ( iSendRet !=  sPackage.size())
    {
        pBuff->SetWriteIndex( pBuff->GetWriteIndex() );
        return RET_RECV_ERR;
    }
    MTS_LOG_DEBUG("after copy to write data, pBuff->ReadableBytes()=%u, to writen data len: %d", 
                  pBuff->ReadableBytes(), iSendRet);

    pBuff->Compact(8*1024);
    return RET_RECV_OK;
}

CODERET BusiCodec::DeCode(CBuffer* pBuff, rapidjson::Document& root)
{
    MTS_LOG_DEBUG("%s() ReadableBytes()=%d, GetReadIndex()=%d", 
                 __FUNCTION__, pBuff->ReadableBytes(), pBuff->GetReadIndex());

    std::string sData(pBuff->GetRawReadBuffer(), pBuff->ReadableBytes());
    std::string::size_type packDiv = sData.find("\n");
    if (packDiv == std::string::npos)
    {
        return RET_RECV_NOT_COMPLETE;
    }

    std::string sPackage = sData.substr(0, packDiv);
    pBuff->SkipBytes(packDiv+1);
    pBuff->Compact(8*1024);

    try 
    {
        root.Parse<0>(sPackage.c_str());
        if (root.HasParseError())
        {
            MTS_LOG_ERROR("recv package not json format, package:  %s", sPackage.c_str());
            return RET_RECV_ERR;
        }
    } 
    catch (...)
    {
        MTS_LOG_ERROR("parse json package exception, package: %s", sPackage.c_str());
        return RET_RECV_ERR;
    }

    MTS_LOG_DEBUG("recv pkg: %s", sPackage.c_str());
    return RET_RECV_OK;
}
