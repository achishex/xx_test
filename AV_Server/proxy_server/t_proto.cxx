#include "t_proto.hxx"
#include <arpa/inet.h>
#include <string.h>
#include "rutil/Logger.hxx"


#include "rapidjson/writer.h"                                                                                       
#include "rapidjson/stringbuffer.h"                                                                                 
#include "rapidjson/document.h"                                                                                     
#include "rapidjson/error/en.h"                                                                                     
#include "rapidjson/prettywriter.h" 


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;
using namespace rapidjson;

namespace repro
{
    //each package divide by "\n", each package format is json
    BusiCodec::BusiCodec(int iType) :m_iCodeType(iType)
    {
    }

    BusiCodec::~BusiCodec()
    {
    }

    CODERET BusiCodec::EnCode(CBuffer* pBuff, rapidjson::Document& root)
    {
        DebugLog(<< __FUNCTION__ << "() ReadableBytes()=" << pBuff->ReadableBytes());
        if (pBuff == NULL)
        {
            ErrLog(<<"data buf is null");
            return RET_RECV_ERR;
        }

        rapidjson::StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        root.Accept(writer);
        std::string sPackage = buffer.GetString();
        sPackage.append("\n");

        unsigned int iSendRet = pBuff->Write(sPackage.c_str(), sPackage.size());
        if ( iSendRet !=  sPackage.size())
        {
            pBuff->SetWriteIndex( pBuff->GetWriteIndex() );
            return RET_RECV_ERR;
        }

        pBuff->Compact(8*1024);
        return RET_RECV_OK;
    }

    CODERET BusiCodec::DeCode(CBuffer* pBuff, rapidjson::Document& root)
    {
        DebugLog(<< __FUNCTION__ << "() ReadableBytes()=" << pBuff->ReadableBytes()
			<< ", GetReadIndex()=" << pBuff->GetReadIndex());

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
                ErrLog( << "recv package not json format, package:  %s" << sPackage.c_str());
                return RET_RECV_ERR;
            }
        } 
        catch (...)
        {
            ErrLog(<< "parse json package exception, package: %s" << sPackage.c_str());
            return RET_RECV_ERR;
        }
        return RET_RECV_OK;
    }
}
