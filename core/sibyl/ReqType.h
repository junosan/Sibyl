/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_REQTYPE_H_
#define SIBYL_REQTYPE_H_

// Req is sent from a client (rnnclnt, refclnt) to a server (KiwoomAgent, simserv) with syntax:
//
// b 000660 36000 2
// [type] [code] [price] [quant]
//     type     b, s, cb, cs
//     code     string
//     price    > 0
//     quant    use 0 for max orders to buy/sell/cancel at [price]
//              otherwise uses designated quantity
//
// mb 000660 36000 2 36500
// [type] [code] [oldprice] [quant] [newprice]
//     type     mb, ms
//     code     string
//     oldprice > 0
//     quant    use 0 to modify all orders at [price]
//              otherwise uses designated quantity
//     newprice > 0, newprice =/= oldprice
//
// ca
//     cancel all orders
//
// sa
//     sell all shares at ps0
//
// Notes:
//     Delimiter for each field is ‘ ‘
//     Delimiter for each request is ’\n’ (“\r\n” is acceptable)
//     Multiple requests can be sent as multiple lines of a message
//     The last character in a message MUST be ‘\n’ (for proper packet concatenation)
//     If there is no order to send, send “\n”

#include <iostream>

namespace sibyl
{

enum class ReqType : long
{
    null = 0,
    b    = 1, // buy            * value specified in Kiwoom API
    s    = 2, // sell           * value specified in Kiwoom API
    cb   = 3, // cancel buy     * value specified in Kiwoom API
    cs   = 4, // cancel sell    * value specified in Kiwoom API
    mb   = 5, // modify buy     * value specified in Kiwoom API
    ms   = 6, // modify sell    * value specified in Kiwoom API
    ca      , // cancel all
    sa        // sell all
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