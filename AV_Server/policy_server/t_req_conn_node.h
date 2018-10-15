/**
 * @file: t_req_conn_node.h
 * @brief:  定义请求连接信息
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-10-14
 */

#ifndef _T_REQ_CONN_NODE_H_
#define _T_REQ_CONN_NODE_H_

#include <string>

typedef struct ConnReqNode
{
    int         m_iAccptFd;
    std::string m_sPeerIp;
    int         m_iPeerPort;
    int         m_tmAccept;
    ConnReqNode(int fd, const std::string& sip, int iport, int tmaccept)
        :m_iAccptFd(fd), m_sPeerIp(sip), m_iPeerPort(iport), m_tmAccept(tmaccept)
    {
    }

    ConnReqNode():m_iAccptFd(0), m_sPeerIp(""), m_iPeerPort(0), m_tmAccept(0)
    {
    }
    virtual ~ConnReqNode()
    {
    }

    std::string ToString()
    {
        std::ostringstream os;
        os << "accept fd: " << m_iAccptFd
            << ", peerip: " << m_sPeerIp
            << ", peerport: " << m_iPeerPort
            << ", tmaccept: " << m_tmAccept;

        return os.str();
    }
} ConnNodeType;
#endif
