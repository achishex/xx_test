/**
 * @file: t_proto.hpp
 * @brief:  
 *       提供简单编解码的接口，如果存在多种协议，可以进一步把接口全部virtual,
 *       业务统一使用virtual 接口，新增协议只要分配注册新实例。
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-04-24
 */

#ifndef _T_PROTO_HPP_
#define _T_PROTO_HPP_

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/prettywriter.h" 

#include "CBuffer.hpp"
#include "avs_mts_log.h"

enum CODERET 
{
    RET_RECV_OK             = 0,
    RET_RECV_NOT_COMPLETE   = 1,
    RET_RECV_ERR            = 2,
};
class BusiCodec
{
 public:
  BusiCodec(int iType);
  virtual ~BusiCodec();
  virtual CODERET DeCode(CBuffer* pBuff, rapidjson::Document& root);

  /**
   * @brief: EnCode 
   *    将 长度为iSrcLen的pSrcBuf编码到 空间 pDstMsg, 最后总的编码空间长度存放
   *    在 piDstMsgLen.
   * @param pSrcBuf
   * @param iSrcLen
   * @param pDstMsg
   * @param piDstMsgLen
   *   既是入参，也是出参，入参作为编码空间的初始偏移量，出参是编码完后，最
   *   终的编码空间的偏移量
   * @return 
   */
  virtual CODERET EnCode(CBuffer* pBuff, rapidjson::Document& root);
 private:
  int m_iCodeType;
};
#endif

