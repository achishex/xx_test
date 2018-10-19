#include "t_report_load.h"

LoadReport::LoadReport(MtsTcp* pMtsTcp)
    :TcpClient(pMtsTcp->GetEventBase()),
    m_pLocalTcpSrv(pMtsTcp) 
{
}

LoadReport::~LoadReport()
{
}
