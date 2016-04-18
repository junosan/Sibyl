#ifndef SIBYL_SERVER_KIWOOM_KIWOOMFUNCTION_H_
#define SIBYL_SERVER_KIWOOM_KIWOOMFUNCTION_H_

#include "Kiwoom_data.h"

namespace sibyl
{

// Wrapper class for Kiwoom's OpenAPI wrapper functions
// These wrapper functions (implemented in Dlg) handle accno, scrno, std->MFC, EUC-KR issues 
class KiwoomFunction
{
public:
    // called by Kiwoom's constructor
    static void SetOrderBook   (OrderBook<OrderKw, ItemKw> *pob_) { pob = pob_; }
    // called by Dlg's constructor
    static void SetWrapperFuncs( void  (*SetInputValue_)  (InputKey, CSTR&),
                                 long  (*CommRqData_)     (CSTR&, CSTR&, bool),
                                 CSTR& (*GetCommData_)    (CSTR&, CSTR&, long, CommDataKey),
                                 long  (*SendOrder_)      (CSTR&, ReqType, CSTR&, PQ, CSTR&),
                                 long  (*SetRealReg_)     (CSTR&, CSTR&, CSTR&),
                                 CSTR& (*GetCommRealData_)(CSTR&, long),
                                 CSTR& (*GetChejanData_)  (long) )           {
                                         SetInputValue   = SetInputValue_;
                                         CommRqData      = CommRqData_;
                                         GetCommData     = GetCommData_;
                                         SendOrder       = SendOrder_;
                                         SetRealReg      = SetRealReg_;
                                         GetCommRealData = GetCommRealData_;
                                         GetChejanData   = GetChejanData_;   }

protected:
    static OrderBook<OrderKw, ItemKw> *pob;
    
    // [ TR-related ]
    // called by Windows msg loop or NetServer thread (during exit)
    static void  (*SetInputValue)   (InputKey key, CSTR &val);
        // InputKey::accno      : use pre-obtained accno in Dlg
    static long  (*CommRqData)      (CSTR &TR_name, CSTR &TR_code, bool carry);
    static CSTR& (*GetCommData)     (CSTR &TR_name, CSTR &TR_code, long idx, CommDataKey key);
        // CommDataKey::code_g  : strip first character
        // CommDataKey::ordtype : convert EUC-KR string to "+1"/"-1" (use enum OrdType)
        
    // called by NetServer thread (reqs)
    static long  (*SendOrder)       (CSTR &TR_name, ReqType type, CSTR &code, PQ pq, CSTR &ordno_o);
    
    // [ RT-related ]
    static long  (*SetRealReg)      (CSTR &scrno, CSTR &codes, CSTR &FIDs);
    static CSTR& (*GetCommRealData) (CSTR &code, long FID);
    static CSTR& (*GetChejanData)   (long FID);
        // kFID::code           : strip first character
        // kFID::reqstat        : convert EUC-KR string to "0"/"1"/"2" (use enum ReqStat)
        // kFID::reqtype        : convert EUC-KR string to "1"/"2"/"3"/"4"/"5"/"6" (use enum ReqType)
};

}

#endif /* SIBYL_SERVER_KIWOOM_KIWOOMFUNCTION_H_ */