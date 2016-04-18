#ifndef SIBYL_SERVER_KIWOOM_RT_H_
#define SIBYL_SERVER_KIWOOM_RT_H_

#include "KiwoomFunction.h"

namespace sibyl
{

// Designed set of KiwoomFunction operations to implement real time event processing
// (i.e., asynchronous market updates & events from my order requests/cnts)
class RT : public KiwoomFunction
{
public:
    // calls SetRealReg based on OrderBook's content
    static bool Start();
    
    // Dlg holds map_STR_RT* and calls virtual functions
    // RealData   have keys "주식체결"/"주식호가잔량"/"ETF NAV" (== OnReceiveRealData's sRealType)
    // ChejanData have keys "RTEvent0"/"RTEvent1" (== "RTEvent" + OnReceiveChejanData's sGubun)
    virtual void Receive(CSTR &code) = 0;
        // code : sRealKey in OnReceiveRealData
        //        GetChejanData(kFID::code) in OnReceiveChejanData
};

class RTMarketTrade : public RT
{
private:
    void Receive(CSTR &code) override {
        
    }
};

class RTMarketTable : public RT
{
private:
    void Receive(CSTR &code) override {
        
    }
};

class RTMarketNAV : public RT
{
private:
    void Receive(CSTR &code) override {
        
    }
};

class RTEventReq : public RT
{
private:
    void Receive(CSTR &code) override {
        
    }
};

class RTEventCnt : public RT
{
private:
    void Receive(CSTR &code) override {
        
    }
};

}

#endif /* SIBYL_SERVER_KIWOOM_RT_H_ */