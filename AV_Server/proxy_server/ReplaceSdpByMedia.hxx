#if !defined(RESIP_REPLACE_SDP_BY_MEDIA_HXX)
#define RESIP_REPLACE_SDP_BY_MEDIA_HXX

#include <sys/types.h>

#include <list>
#include <vector>
#include <utility>
#include <memory> 

#include "resip/stack/Contents.hxx"
#include "resip/stack/Headers.hxx"
#include "resip/stack/TransactionMessage.hxx"
#include "resip/stack/ParserContainer.hxx"
#include "resip/stack/ParserCategories.hxx"
#include "resip/stack/SecurityAttributes.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/MessageDecorator.hxx"
#include "resip/stack/Cookie.hxx"
#include "resip/stack/WsCookieContext.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "rutil/DinkyPool.hxx"
#include "rutil/StlPoolAllocator.hxx"
#include "rutil/Timer.hxx"
#include "rutil/HeapInstanceCounter.hxx"
#include "rutil/SharedPtr.hxx"

#include "NotifyPrivateMesage.hxx"

namespace resip
{

class Contents;

}

namespace repro
{
class SdpReplaceMedia
{
    public:
     SdpReplaceMedia(resip::Contents* pCntext, MediaPortSource* pMedia, bool req = true);
     virtual ~SdpReplaceMedia();
     bool Replace();

    private:
     bool SetAudioMediaLine( SdpContents::Session::Medium& media );
     bool SetVideoMediaLine( SdpContents::Session::Medium& media );
     bool SetMediaLine( SdpContents::Session::Medium& media );
     bool ProcAttrRtcp( SdpContents::Session::Medium& media );

    private:
     Contents* m_pCntents;
     MediaPortSource* m_pMedia;
     bool m_isReq;
};

}

#endif
