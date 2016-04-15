#ifndef SIBYL_SERVER_KIWOOM_KIWOOM_H_
#define SIBYL_SERVER_KIWOOM_KIWOOM_H_

#include <atomic>

#include "Kiwoom_data.h"
#include "../Broker.h"
#include "../../util/Clock.h"

namespace sibyl
{

class Kiwoom : public Broker<OrderKw, ItemKw> // /**/ locks orderbook.items_mutex
{
public:
    static Clock clock;
    
    // Wrapper function pointers for OpenAPI's TR-related functions
    // called by Windows msg loop (initialization) or NetServer thread (during exit)
    static void  (*SetInputValue)(InputKey key, CSTR &val);
    static long  (*CommRqData)   (CSTR &TR_name, CSTR &TR_code, long cont);
    static CSTR& (*GetCommData)  (CSTR &TR_name, CSTR &TR_code, CommDataKey key);
    // called by NetServer thread
    static long  (*SendOrder)    (CSTR &TR_name, ReqType type, CSTR &code, PQ pq, CSTR &ordno_o);

    // called in Dlg's constructor
    void SetStateFile(CSTR &filename);
    void SetDelay(int d) { timeOffset = TimeBounds::null - d; }
    void SetWrapperFuncs( void  (*SetInputValue_)(InputKey, CSTR&),
                          long  (*CommRqData_)   (CSTR&, CSTR&, long),
                          CSTR& (*GetCommData_)  (CSTR&, CSTR&, CommDataKey),
                          long  (*SendOrder_)    (CSTR&, ReqType, CSTR&, PQ, CSTR&) ) {
                                  SetInputValue = SetInputValue_;
                                  CommRqData    = CommRqData_;
                                  GetCommData   = GetCommData_;
                                  SendOrder     = SendOrder_;
                          }

    // called by NetServer thread
    int   AdvanceTick() override;
/**/CSTR& BuildMsgOut() override;
/**/void  WriteState ();
    
    // called by any thread
    // Note: although orderbook.time is public, use GetOrderBookTime() instead (atomic)
    int GetOrderBookTime() { return timeOrderBook; }
    
    // called by OpenAPI event thread
/**/void ApplyRealtimeTr (CSTR &code, INT p, INT q, INT trPs1, INT trPb1);
/**/void ApplyRealtimeTb (CSTR &code, CSTR &time, const std::array<PQ, szTb> &tb);
/**/void ApplyRealtimeNAV(CSTR &code, INT p, FLOAT nav);
/**/void ApplyReqEvent   (CSTR &code, ReqStat reqStat, ReqType reqType,
                          CSTR &ordno, INT ordp, INT ordq,
                          CSTR &ordno_o, INT delta_p, INT delta_q);
/**/void ApplyCntEvent   (CSTR &code, INT cnt); // check integrity with ApplyReqEvent
    
    Kiwoom() : timeOffset(TimeBounds::null),
               t_data_minus_local(0)
               { SetOrderBookTime(-3600); } // starts at 08:00:10
private:
    STR stateFileName;
    
    int timeOffset; // second; 09:00:00-based time - timeOffset = 00:00:00-based time
    
    std::atomic_int t_data_minus_local; // millisecond difference between tb data time & local time
    std::atomic_int timeOrderBook;      // second; updated whenever orderbook.time is changed
    
    // called by NetServer thread
    void SetOrderBookTime(int t) { timeOrderBook = orderbook.time = t; }

    // called by NetServer thread
    int  ExecuteNamedReq(NamedReq<OrderKw, ItemKw> req) override; // non-0 if req count overflow
    
    // called by OpenAPI event thread
/**/it_itm_t<ItemKw>  FindItem (CSTR &code);
/**/it_ord_t<OrderKw> FindOrder(it_itm_t<ItemKw> it_itm, INT p, CSTR &ordno);
};

}

#endif /* SIBYL_SERVER_KIWOOM_KIWOOM_H_ */