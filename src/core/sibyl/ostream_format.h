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

#ifndef SIBYL_OSTREAM_FORMAT_H_
#define SIBYL_OSTREAM_FORMAT_H_

#include <iostream>
#include <iomanip>

#include "sibyl_common.h"

namespace sibyl
{

// General formatting helper classes
// Feed ostream with fmt_price(36000), fmt_code("000660"), etc. for consistent formatting/alignment
// Decoration convention: (quant), {code}, #ordno

// struct name_ {
//     constexpr static int w =  w_;
//     vtype_ val;
//     name_ (itype_ in) : val(in isuffix_) {}
//  };
#define FMT_DEFINE(name_, w_, vtype_, itype_, isuffix_) struct name_ { constexpr static int w = w_; vtype_ val; name_ (itype_ in) : val(in isuffix_) {} };

FMT_DEFINE(fmt_price,  8, INT        , INT  ,         );
FMT_DEFINE(fmt_bal  , 12, INT64      , INT64,         );
FMT_DEFINE(fmt_quant,  8, INT        , INT  ,         );
FMT_DEFINE(fmt_code ,  8, const char*, CSTR&, .c_str());
FMT_DEFINE(fmt_ordno,  8, const char*, CSTR&, .c_str());

#undef FMT_DEFINE

inline std::ostream& operator<<(std::ostream &os, const fmt_price &in) {
    return os << std::setw(fmt_price::w) << in.val;
}

inline std::ostream& operator<<(std::ostream &os, const fmt_bal   &in) {
    return os << std::setw(fmt_bal::w) << in.val;
}

inline std::ostream& operator<<(std::ostream &os, const fmt_quant &in) {
    return os << "(" << std::setw(fmt_quant::w - 2) << in.val << ")";
}

inline std::ostream& operator<<(std::ostream &os, const fmt_code  &in) {
    return os << "{" << std::setw(fmt_code::w - 2) << in.val << "}";
}

inline std::ostream& operator<<(std::ostream &os, const fmt_ordno &in) {
    return os << "#" << std::setw(fmt_ordno::w - 1) << in.val;
}

}

#endif /* SIBYL_OSTREAM_FORMAT_H_ */