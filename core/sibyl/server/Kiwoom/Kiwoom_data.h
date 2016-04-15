#ifndef SIBYL_SERVER_KIWOOM_KIWOOM_DATA_H_
#define SIBYL_SERVER_KIWOOM_KIWOOM_DATA_H_

#include "../OrderBook.h"

namespace sibyl
{

class OrderKw : public Order
{
public:
    STR ordno;
};

class ItemKw : public Item<OrderKw>
{
public:
    INT64 sumQ, sumPQ;
    INT trPs1, trPb1;
    std::array<PQ, szTb> tb; // raw realtime table
    STR srcno;
    ItemKw() : sumQ(0), sumPQ(0), trPs1(0), trPb1(0) {}
};

enum class ReqStat
{
    received,
    confirmed,
    traded
};

enum class FID : int
{
    code    = 9001, // *###### (*: A(spot), J(ELW), Q(ETN)), some_other_format (future/option)
    cnt     = 933,
    reqstat = 913,  // ReqStat (in Korean)
    reqtype = 905,  // b, s, cb, cs, mb, ms (in Korean)
    ordno   = 9203,
    ordp    = 901,
    ordq    = 902,
    ordno_o = 904,
    delta_p = 914,
    delta_q = 915
};

enum class InputKey
{
    code,        // 종목코드
    accno,       // 계좌번호 (need separate handling; use pre-obtained accno in Dlg)
    pin,         // 비밀번호
    pin_input_s, // 비밀번호입력매체구분
    query_s,     // 조회구분
    reqstat_s,   // 체결구분
    ordtype_s    // 매매구분    
};

enum class CommDataKey
{
    code,        // 종목코드
    code_g,      // 종목번호 (need separate handling; strip first character)
    refprice,    // 기준가
    delayedbal,  // d+2출금가능금액
    cnt_plus_so, // 보유수량
    reqtype,     // 주문구분 (need separate handling; convert to b/s/cb/cs/mb/ms)
    ordno,       // 주문번호
    ordp,        // 주문가격
    ordq         // 미체결수량
};

}

#endif /* SIBYL_SERVER_KIWOOM_KIWOOM_DATA_H_ */