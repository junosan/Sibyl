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

enum class ErrorCode : long
{
    OP_ERR_NONE            =    0, // "정상처리"
    OP_ERR_LOGIN           = -100, // "사용자정보교환에 실패하였습니다. 잠시후 다시 시작하여 주십시오."
    OP_ERR_CONNECT         = -101, // "서버 접속 실패"
    OP_ERR_VERSION         = -102, // "버전처리가 실패하였습니다."

    OP_ERR_SISE_OVERFLOW   = -200, // "시세조회 과부하"
    OP_ERR_RQ_STRUCT_FAIL  = -201, // "REQUEST_INPUT_st Failed"
    OP_ERR_RQ_STRING_FAIL  = -202, // "요청 전문 작성 실패"
    OP_ERR_NO_DATA         = -203, // "데이터가 존재하지 않습니다."
    OP_ERR_OVER_MAX_DATA   = -204, // "한번에 조회 가능한 종목개수는 최대 100종목 입니다."

    OP_ERR_ORD_WRONG_INPUT = -300, // "입력값 오류"
    OP_ERR_ORD_WRONG_ACCNO = -301, // "계좌비밀번호를 입력하십시오."
    OP_ERR_OTHER_ACC_USE   = -302, // "타인계좌는 사용할 수 없습니다."
    OP_ERR_MIS_2BILL_EXC   = -303, // "주문가격이 20억원을 초과합니다."
    OP_ERR_MIS_5BILL_EXC   = -304, // "주문가격이 50억원을 초과합니다."
    OP_ERR_MIS_1PER_EXC    = -305, // "주문수량이 총발행주수의 1%를 초과합니다."
    OP_ERR_MIS_3PER_EXC    = -306, // "주문수량은 총발행주수의 3%를 초과할 수 없습니다."
    OP_ERR_SEND_FAIL       = -307, // "주문전송실패"
    OP_ERR_ORD_OVERFLOW    = -308  // "주문전송 과부하"
};

}

#endif /* SIBYL_SERVER_KIWOOM_KIWOOM_DATA_H_ */