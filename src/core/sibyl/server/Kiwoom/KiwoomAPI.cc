/*
   Copyright 2017 Hosang Yoon

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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
