/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_SERVER_KIWOOM_KIWOOM_H_
#define SIBYL_SERVER_KIWOOM_KIWOOM_H_

#include <atomic>

#include "Kiwoom_data.h"
#include "KiwoomAPI.h"
#include "../Broker.h"
#include "../../ReqType.h"
#include "../../time_common.h"
#include "TR.h"

namespace sibyl
{

class Kiwoom : public Broker<OrderKw, ItemKw> // /**/ locks orderbook.items_mutex
{
public:
    // called by Windows msg loop (initialization)
    void SetStateFile(CSTR &filename);
    void ReadConfigFiles(CSTR &config);//, CSTR &codelist);
    bool Launch(); // retruns true if successful (then NetServer::StartMainLoop)

    // called by NetServer thread
    int   AdvanceTick() override;
/**/CSTR& BuildMsgOut() override;
/**/void  WriteState ();
    
    // Asynchronous data (OnReceiveRealData { code = sRealKey } | OnReceiveChejanData)    
    // called by OpenAPI event thread
/**/void ReceiveMarketTr (CSTR &code); // OnReceiveRealData   sRealType == "stock trade"  (in Korean)
/**/void ReceiveMarketTb (CSTR &code); // OnReceiveRealData   sRealType == "stock table"  (in Korean)
/**/void ReceiveMarketNAV(CSTR &code); // OnReceiveRealData   sRealType == "ETF NAV"   }
/**/void ReceiveIndex    (CSTR &code); // OnReceiveRealData   sRealType == "sector index" (in Korean)   }
/**/void ReceiveOrdEvent ();           // OnReceiveChejanData sGubun    == "0"         }
/**/void ReceiveCntEvent ();           // OnReceiveChejanData sGubun    == "1"         }
    
    // Note: although orderbook.time is public, use GetOrderBookTime() instead (atomic)
    int GetOrderBookTime() const { return orderbook.time; }
    int GetTimeOffset()    const { return timeOffset;     } 
    
    Kiwoom() : timeOffset(kTimeBounds::null),
               t_data_minus_local(0)
               { SetOrderBookTime(-3600);
                 TR::SetOrderBook(&orderbook); } // starts at 08:00:10
private:
    // called by Windows msg loop (initialization)
    void SetDelay(int d) { timeOffset = kTimeBounds::null - d; } // seconds
    
    // called by NetServer thread
    void SetOrderBookTime(int t) { orderbook.time = t; }
/**/int  ExecuteNamedReq(NamedReq<OrderKw, ItemKw> req) override; // non-0 if req count overflow
    void OnExit() override;
    
    // called by OpenAPI event thread
/**/it_itm_t<ItemKw>  FindItem (CSTR &code);
/**/it_ord_t<OrderKw> FindOrder(it_itm_t<ItemKw> it_itm, INT p, CSTR &ordno, bool nowarn = false);

    STR stateFileName;
    int timeOffset; // second; 09:00:00-based time - timeOffset = 00:00:00-based time
    std::atomic_int t_data_minus_local; // millisecond difference between tb data time & local time

    // Make sure mutex is not locked when calling TR.Send(#)!
    // called by Windows msg loop or NetServer thread (after finishing)
    TRRefPrice   trRefPrice;
    TRAccBalance trAccBalance;
    TRCntOList   trCntOList;
    TROrdList    trOrdList;
    TRIndex      trIndex;
    // called by NetServer thread
    TRReqOrder   trReqOrder;
    
    typedef KiwoomAPI K; // for brevity
};

}

#endif /* SIBYL_SERVER_KIWOOM_KIWOOM_H_ */