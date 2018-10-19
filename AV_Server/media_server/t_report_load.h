/**
 * @file: t_report_load.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-08-06
 */

#ifndef _T_REPORT_LOAD_H_
#define _T_REPORT_LOAD_H_

#include "rapidjson/writer.h"                                                                                           
#include "rapidjson/stringbuffer.h"                                                                                     
#include "rapidjson/document.h"                                                                                         
#include "rapidjson/error/en.h"                                                                                         
#include "rapidjson/prettywriter.h" 

#include <map>
#include <string>
#include "t_socket.hpp"
#include "mts_server.h"
#include "t_proto.hpp"
#include "t_eventbase.hpp"
#include "t_conn_data.h"
#include "t_tcp_client.h"

//----------------------------------------------------------------------//
class LoadReport: public TcpClient
{
 public:
  LoadReport( MtsTcp* pMtsTcp );
  virtual ~LoadReport();
 private:
  MtsTcp*   m_pLocalTcpSrv;
};


#endif
