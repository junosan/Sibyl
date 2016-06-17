/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include "KiwoomAPI.h"

namespace sibyl
{

// static
void  (*KiwoomAPI::SetInputValue)  (InputKey, CSTR&)                                = nullptr;
long  (*KiwoomAPI::GetRepeatCnt)   (CSTR&, CSTR&)                                   = nullptr;
long  (*KiwoomAPI::CommRqData)     (CSTR&, CSTR&, bool, CSTR&)                      = nullptr;
CSTR& (*KiwoomAPI::GetCommData)    (CSTR&, CSTR&, long, CommDataKey)                = nullptr;
long  (*KiwoomAPI::SendOrder)      (CSTR&, CSTR&, CSTR&, ReqType, CSTR&, PQ, CSTR&) = nullptr;
long  (*KiwoomAPI::SetRealReg)     (CSTR&, CSTR&, CSTR&)                            = nullptr;
CSTR& (*KiwoomAPI::GetCommRealData)(CSTR&, long)                                    = nullptr;
CSTR& (*KiwoomAPI::GetChejanData)  (long)                                           = nullptr;

}
