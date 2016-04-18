
#include "KiwoomFunction.h"

namespace sibyl
{

// static
OrderBook<OrderKw, ItemKw> *KiwoomFunction::pob                            = nullptr;
void  (*KiwoomFunction::SetInputValue)  (InputKey, CSTR&)                  = nullptr;
long  (*KiwoomFunction::CommRqData)     (CSTR&, CSTR&, bool)               = nullptr;
CSTR& (*KiwoomFunction::GetCommData)    (CSTR&, CSTR&, long, CommDataKey)  = nullptr;
long  (*KiwoomFunction::SendOrder)      (CSTR&, ReqType, CSTR&, PQ, CSTR&) = nullptr;
long  (*KiwoomFunction::SetRealReg)     (CSTR&, CSTR&, CSTR&)              = nullptr;
CSTR& (*KiwoomFunction::GetCommRealData)(CSTR&, long)                      = nullptr;
CSTR& (*KiwoomFunction::GetChejanData)  (long)                             = nullptr;

}
