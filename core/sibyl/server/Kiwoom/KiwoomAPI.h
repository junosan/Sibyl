#ifndef SIBYL_SERVER_KIWOOM_KIWOOMAPI_H_
#define SIBYL_SERVER_KIWOOM_KIWOOMAPI_H_

#include "Kiwoom_data.h"

namespace sibyl
{

// Wrapper class for Kiwoom's OpenAPI wrapper functions
// These wrapper functions (implemented in Dlg) handle std->MFC, EUC-KR issues 
class KiwoomAPI
{
public:
    // called by Dlg's constructor
    static void SetWrapperFuncs( void  (*SetInputValue_)  (InputKey, CSTR&),
                                 long  (*GetRepeatCnt_)   (CSTR&, CSTR&),
                                 long  (*CommRqData_)     (CSTR&, CSTR&, bool, CSTR&),
                                 CSTR& (*GetCommData_)    (CSTR&, CSTR&, long, CommDataKey),
                                 long  (*SendOrder_)      (CSTR&, CSTR&, CSTR&, ReqType, CSTR&, PQ, CSTR&),
                                 long  (*SetRealReg_)     (CSTR&, CSTR&, CSTR&),
                                 CSTR& (*GetCommRealData_)(CSTR&, long),
                                 CSTR& (*GetChejanData_)  (long) )           {
                                         SetInputValue   = SetInputValue_;
                                         GetRepeatCnt    = GetRepeatCnt_;
                                         CommRqData      = CommRqData_;
                                         GetCommData     = GetCommData_;
                                         SendOrder       = SendOrder_;
                                         SetRealReg      = SetRealReg_;
                                         GetCommRealData = GetCommRealData_;
                                         GetChejanData   = GetChejanData_;   }

    // [ TR-related ]
    // called by Windows msg loop or NetServer thread (during exit)
    static void  (*SetInputValue)   (InputKey key, CSTR &val);
    static long  (*GetRepeatCnt)    (CSTR &TR_name, CSTR &TR_code);
    static long  (*CommRqData)      (CSTR &TR_name, CSTR &TR_code, bool carry, CSTR &scrno);
    static CSTR& (*GetCommData)     (CSTR &TR_name, CSTR &TR_code, long idx, CommDataKey key);
        // CommDataKey::code_g  : strip first character
        // CommDataKey::ordtype : convert EUC-KR string to "1"/"-1" (use enum OrdType)
        
    // called by NetServer thread (reqs)
    static long  (*SendOrder)       (CSTR &TR_name, CSTR &scrno, CSTR &accno, ReqType type, CSTR &code, PQ pq, CSTR &ordno_o);
    
    // [ RT-related ]
    static long  (*SetRealReg)      (CSTR &scrno, CSTR &codes, CSTR &FIDs);
    static CSTR& (*GetCommRealData) (CSTR &code, long FID);
    static CSTR& (*GetChejanData)   (long FID);
        // kFID::code           : strip first character
        // kFID::reqstat        : convert EUC-KR string to "0"/"1"/"2" (use enum ReqStat)
        // kFID::reqtype        : convert EUC-KR string to "1"/"2"/"3"/"4"/"5"/"6" (use enum ReqType)
};

}

#endif /* SIBYL_SERVER_KIWOOM_KIWOOMAPI_H_ */