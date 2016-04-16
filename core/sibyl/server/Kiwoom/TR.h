#ifndef SIBYL_SERVER_KIWOOM_TR_H_
#define SIBYL_SERVER_KIWOOM_TR_H_

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "Kiwoom.h"

namespace sibyl
{

class TR
{
public:
    enum class State { normal, carry, timeout };

    // called by Windows msg loop (initialization) or NetServer thread (reqs)
    State Send();
    // called by OpenAPI event thread (OnReceiveTrData)
    void Receive(long cnt, State state); // cnt: # of idx to loop, state: normal | carry

    // use to initialize name to TR map
    virtual CSTR& Name() = 0;
    virtual CSTR& Code() = 0;

    TR() : ab_end(false) {}
protected:
    virtual bool IsContinuable()        = 0;
    virtual long SendOnce(State state)  = 0; // SetInputValue (if any) & CommRqData/SendOrder
    virtual int  GetTimeout()           = 0;
    virtual void RetrieveData(long cnt) = 0;    
private:
    // called by Windows msg loop (initialization) or NetServer thread (reqs)
    void Init();               // regulate TR rate per sec
    State Wait(int t_timeout); // milliseconds; nonpositive for indefinite wait
    // called by OpenAPI event thread (OnReceiveTrData)
    void End(State state);     // normal | carry
    
    std::condition_variable cv;
    std::mutex              cv_mutex;
    std::atomic_bool        ab_end;
};

}

#endif /* SIBYL_SERVER_KIWOOM_TR_H_ */