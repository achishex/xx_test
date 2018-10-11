/**
 * @file: t_event.hpp
 * @brief:  用于接收client req-connect的连接对象， 该对象主要在accept后创建一个收发
 *          数据的新fd. 该对象一般被isten/bind 端口的线程调用.目前是被主线程调
 *          用.
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-04-23
 */

#ifndef _T_EVENT_LISTEN_HPP_
#define _T_EVENT_LISTEN_HPP_

#include "t_eventbase.hpp"
#include "avs_mts_log.h"

class MtsTcp;

class ListenConn: public ConnBase
{
 public:
  ListenConn(int ifd, void* pData);
  virtual ~ListenConn();
  virtual bool AddEvent(struct event_base *pEvBase, int iEvent, void *arg);

  virtual bool DelEvent(struct event_base *pEvBase, int iEvent, void *arg);
 private:
  static void ReadCallBack(int iFd, short sEvent, void *pData); 
  static void WriteCallBack(int iFd, short sEvent, void *pData); 
  bool DoRead();

 private:
  MtsTcp* m_pData;
};

#endif


