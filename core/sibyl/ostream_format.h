#ifndef SIBYL_OSTREAM_FORMAT_H_
#define SIBYL_OSTREAM_FORMAT_H_

#include <iostream>
#include <iomanip>

#include "sibyl_common.h"

namespace sibyl
{

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