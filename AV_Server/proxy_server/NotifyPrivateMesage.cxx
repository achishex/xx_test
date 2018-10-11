#include "NotifyPrivateMesage.hxx"
#include "monkeys/PushIotServer.hxx"

namespace repro
{

NotifyToInviteChannel::NotifyToInviteChannel(RequestContext& reqContext, AsyncProcessor* asyncproc)
    : m_ReqContext(reqContext), m_pAsyncProccor(asyncproc)
{
}

NotifyToInviteChannel::~NotifyToInviteChannel()
{
}

bool NotifyToInviteChannel::Post(const NotifyMessage& msg)
{
    repro::PushIotServer* async_proc =  dynamic_cast<repro::PushIotServer*>(m_pAsyncProccor);
    if (async_proc)
    {
        DebugLog( << "notify msg to invite, invite tid: " << m_ReqContext.getTransactionId() );
        return async_proc->NotifyMsgPost( msg.vdstContact, m_ReqContext );
    }
    return false;
}

//
}
