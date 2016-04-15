#ifndef SIBYL_SERVER_KIWOOM_TR_H_
#define SIBYL_SERVER_KIWOOM_TR_H_

#include <atomic>
#include <condition_variable>

#include "Kiwoom.h"

namespace sibyl
{

class TR
{
public:
    virtual CSTR& Name() = 0;
    
    // called by Windows msg loop (initialization) or NetServer thread (reqs)
    void Send();
    
    // called by OpenAPI event thread (OnReceiveTrData)
    void Receive(/* some data */);
    
    TR() : ab_end(false) {}
protected:
    virtual bool IsContinuable() = 0;
    virtual long SendOnce() = 0; // SetInputValue (if any) & CommRqData/SendOrder
    
private:
    void Init();
    long Wait(int t_timeout);
    void End();
    
    std::condition_variable cv;
    std::mutex              cv_mutex;
    std::atomic_bool        ab_end;
};

}

#endif /* SIBYL_SERVER_KIWOOM_TR_H_ */