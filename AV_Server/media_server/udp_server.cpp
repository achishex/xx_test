#include    <iostream>
#include    <unistd.h>

#include    "udp_server.h"
#include    "t_PortPools.h"
#include    "t_pthread_manager.h"

#include    <sys/types.h>          /* See NOTES */
#include    <sys/socket.h>
#include    <strings.h>


MtsUdp::MtsUdp(std::shared_ptr<LockQueue<std::shared_ptr<RelayPortRecord>>> queueSession)
    :m_pSessionQue(queueSession), m_bExit(false)
{
}

MtsUdp::~MtsUdp()
{
    this->ShutDown();
}

void MtsUdp::UdpThreadCallback()
{
    std::shared_ptr<RelayPortRecord> oneRecord(nullptr);
    while( !m_bExit )
    {
        oneRecord = nullptr;
        m_pSessionQue->Get(&oneRecord);
        if (oneRecord == nullptr)
        {
            MTS_LOG_ERROR("get session node is null");
            continue;
        }
        
        char* buf = (char*)(oneRecord.get());
        int iLen = sizeof(RelayPortRecord);
        MTS_LOG_DEBUG("udp thread get session node: %s", oneRecord->ToString().c_str());

        PthreadManager::Instance()->DispatchUdpMsg( buf, iLen );
    }
    MTS_LOG_INFO("udp thread main_loop exit");
}
