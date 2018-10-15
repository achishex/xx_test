#ifndef _PROTO_INNER_H_
#define _PROTO_INNER_H_



#define     CMD_NO_REGISTER_SIP_REQ     "register_sip_req"
#define     CMD_NO_REPORT_SIP_REQ       "report_sip_req"    /*复用注册流程 **/
#define     CMD_NO_REGISTER_MTS_REQ     "register_mts_req"
#define     CMD_NO_REPORT_MTS_REQ       "report_mts_req"    /*复用注册流程 **/
#define     CMD_NO_GET_SIP_REQ          "get_sip_req"
#define     CMD_NO_GET_MTS_REQ          "get_mts_req"
#define     CMD_NO_PUSH_IOT             "push_msg"          /*推送IOT消呼叫息*/
#define     CMD_NO_AUTH_PUSH_IOT        "um_sip_call_auth"  /*推送IOT鉴权消息*/

#define     CMD_NO_ALLOCATE_RELAY_PORT_REQ      "allocate_relay_port_req"
#define     CMD_NO_RELEASE_RELAY_PORT_REQ       "release_relay_port_req"


#define IS_SIP_REGISTER(x)   (x== CMD_NO_REGISTER_SIP_REQ)
#define IS_SIP_REPORT(x)     (x==CMD_NO_REPORT_SIP_REQ)
#define IS_GET_SIP(x)        (x==CMD_NO_GET_SIP_REQ)

#define IS_MTS_REGISTER(x)      (x==CMD_NO_REGISTER_MTS_REQ)
#define IS_MTS_REPORT(x)        (x==CMD_NO_REPORT_MTS_REQ)
#define IS_GET_MTS(x)           (x==CMD_NO_GET_MTS_REQ)



#endif
