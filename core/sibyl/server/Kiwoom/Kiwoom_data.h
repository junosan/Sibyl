/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_SERVER_KIWOOM_KIWOOM_DATA_H_
#define SIBYL_SERVER_KIWOOM_KIWOOM_DATA_H_

#include <iostream>
#include <iomanip>

#include "../OrderBook.h"
#include "../NetServer.h"
#include "../../ostream_format.h"

namespace sibyl
{

class OrderKw : public Order
{
public:
    STR ordno;
};

inline std::ostream& operator<<(std::ostream &os, OrderKw o) {
    return os << fmt_ordno(o.ordno) << " " << o.type << " " << fmt_price(o.p) << " " << fmt_quant(o.q);
}

class ItemKw : public Item<OrderKw>
{
public:
    INT64 sumQ, sumPQ;
    INT trPs1, trPb1;
    std::array<PQ, idx::szTb> tb; // raw realtime table
    STR srcno;
    ItemKw() : sumQ(0), sumPQ(0), trPs1(0), trPb1(0) {}
};

typedef NetServer<OrderKw, ItemKw> KiwoomServer;

enum class ReqStat
{
    received  = 0,
    confirmed = 1,
    traded    = 2
};

enum class InputKey
{
    code,        // 
    accno,       // 
    pin,         // 
    pin_input_s, // 
    query_s,     // 
    reqstat_s,   // 
    ordtype_s    //     
};

enum class CommDataKey
{
    code,        // 
    code_g,      // (need separate handling; strip first character)
    refprice,    // 
    delayedbal,  // 
    cnt_plus_so, // 
    ordtype,     // (need separate handling; convert to "1"/"-1" for buy/sell)
    ordno,       // 
    ordp,        // 
    ordq         // 
};

enum class MarketEventType
{
    null,
    trade,
    table,
    NAV
};

enum class BookEventType : long
{
    ord = 0, // value specified in Kiwoom API
    cnt = 1  // value specified in Kiwoom API
};

namespace kFID
{
    // OnReceiveChejanData
    constexpr long code    = 9001; // *###### (*: A(spot), J(ELW), Q(ETN)), some_other_format (future/option)
    constexpr long cnt     = 933;
    constexpr long reqstat = 913;  // "receipt"/"confirm"/"trade" (in EUC-KR)
    constexpr long reqtype = 905;  // "+b"/"-s"/"cb"/"cs"/"+mb"/"-ms" (in EUC-KR)
    constexpr long ordno   = 9203;
    constexpr long ordp    = 901;
    constexpr long ordq    = 902;  // currently left
    constexpr long ordno_o = 904;
    constexpr long delta_p = 914;
    constexpr long delta_q = 915;
    constexpr long ordq_i  = 900;  // initially ordered
    
    // OnReceiveRealData
    constexpr long tr_t    = 20; // = nav_t
    constexpr long tr_p    = 10; // = nav_p
    constexpr long tr_q    = 15;
    constexpr long tr_ps1  = 27;
    constexpr long tr_pb1  = 28;
    constexpr long tb_t    = 21;
    constexpr long tb_ps1  = 41; // ps10 = 50
    constexpr long tb_pb1  = 51; // pb10 = 60
    constexpr long tb_qs1  = 61; // qs10 = 70
    constexpr long tb_qb1  = 71; // qb10 = 80
    constexpr long nav_NAV = 36;
};

namespace kKiwoomError
{
    constexpr long OP_ERR_NONE            =    0; // 
    constexpr long OP_ERR_LOGIN           = -100; // 
    constexpr long OP_ERR_CONNECT         = -101; // 
    constexpr long OP_ERR_VERSION         = -102; // 

    constexpr long OP_ERR_SISE_OVERFLOW   = -200; // 
    constexpr long OP_ERR_RQ_STRUCT_FAIL  = -201; // 
    constexpr long OP_ERR_RQ_STRING_FAIL  = -202; // 
    constexpr long OP_ERR_NO_DATA         = -203; // 
    constexpr long OP_ERR_OVER_MAX_DATA   = -204; // 

    constexpr long OP_ERR_ORD_WRONG_INPUT = -300; // 
    constexpr long OP_ERR_ORD_WRONG_ACCNO = -301; // 
    constexpr long OP_ERR_OTHER_ACC_USE   = -302; // 
    constexpr long OP_ERR_MIS_2BILL_EXC   = -303; // 
    constexpr long OP_ERR_MIS_5BILL_EXC   = -304; // 
    constexpr long OP_ERR_MIS_1PER_EXC    = -305; // 
    constexpr long OP_ERR_MIS_3PER_EXC    = -306; // 
    constexpr long OP_ERR_SEND_FAIL       = -307; // 
    constexpr long OP_ERR_ORD_OVERFLOW    = -308; // 
};

}

#endif /* SIBYL_SERVER_KIWOOM_KIWOOM_DATA_H_ */