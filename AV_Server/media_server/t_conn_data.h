#ifndef _T_CONN_DATA_H_
#define _T_CONN_DATA_H_

struct tagConnFd
{
    int iFd;
    unsigned int uiSeq;

    tagConnFd(): iFd(-1), uiSeq(0)
    {  }

    tagConnFd(const tagConnFd& item)
    {
        iFd     = item.iFd;
        uiSeq   = item.uiSeq;
    }
    tagConnFd& operator = ( const tagConnFd& item )
    {
        if (this != &item)
        {
            iFd     = item.iFd;
            uiSeq   = item.uiSeq;
        }
        return *this;
    }
};



#endif
