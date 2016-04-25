#ifndef SIBYL_REQTYPE_H_
#define SIBYL_REQTYPE_H_

#include <iostream>

namespace sibyl
{

enum class ReqType : long
{
    null = 0,
    b    = 1, // value specified in Kiwoom API
    s    = 2, // value specified in Kiwoom API
    cb   = 3, // value specified in Kiwoom API
    cs   = 4, // value specified in Kiwoom API
    mb   = 5, // value specified in Kiwoom API
    ms   = 6, // value specified in Kiwoom API
    ca      ,
    sa
};

inline std::ostream& operator<<(std::ostream &os, ReqType type)
{
    switch (type)
    {
        case ReqType::null : return os << "null";
        case ReqType::b    : return os << " b";
        case ReqType::s    : return os << " s";
        case ReqType::cb   : return os << "cb";
        case ReqType::cs   : return os << "cs";
        case ReqType::mb   : return os << "mb";
        case ReqType::ms   : return os << "ms";
        case ReqType::ca   : return os << "ca";
        case ReqType::sa   : return os << "sa";   
    }
    return os;
}

}

#endif /* SIBYL_REQTYPE_H_ */