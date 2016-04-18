#ifndef SIBYL_SERVER_KIWOOM_KIWOOM_DATA_H_
#define SIBYL_SERVER_KIWOOM_KIWOOM_DATA_H_

#include "../OrderBook.h"
#include "../NetServer.h"

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
    ordtype,     // 주문구분 (need separate handling; convert to "+1"/"-1" for buy/sell)
    ordno,       // 주문번호
    ordp,        // 주문가격
    ordq         // 미체결수량
};

namespace kFID
{
    // OnReceiveChejanData
    constexpr long code    = 9001; // *###### (*: A(spot), J(ELW), Q(ETN)), some_other_format (future/option)
    constexpr long cnt     = 933;
    constexpr long reqstat = 913;  // "접수"/"확인"/"체결" (in EUC-KR)
    constexpr long reqtype = 905;  // "+매수"/"-매도"/"매수취소"/"매도취소"/"+매수정정"/"-매도정정" (in EUC-KR)
    constexpr long ordno   = 9203;
    constexpr long ordp    = 901;
    constexpr long ordq    = 902;
    constexpr long ordno_o = 904;
    constexpr long delta_p = 914;
    constexpr long delta_q = 915;
    
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
    constexpr long OP_ERR_NONE            =    0; // "정상처리"
    constexpr long OP_ERR_LOGIN           = -100; // "사용자정보교환에 실패하였습니다. 잠시후 다시 시작하여 주십시오."
    constexpr long OP_ERR_CONNECT         = -101; // "서버 접속 실패"
    constexpr long OP_ERR_VERSION         = -102; // "버전처리가 실패하였습니다."

    constexpr long OP_ERR_SISE_OVERFLOW   = -200; // "시세조회 과부하"
    constexpr long OP_ERR_RQ_STRUCT_FAIL  = -201; // "REQUEST_INPUT_st Failed"
    constexpr long OP_ERR_RQ_STRING_FAIL  = -202; // "요청 전문 작성 실패"
    constexpr long OP_ERR_NO_DATA         = -203; // "데이터가 존재하지 않습니다."
    constexpr long OP_ERR_OVER_MAX_DATA   = -204; // "한번에 조회 가능한 종목개수는 최대 100종목 입니다."

    constexpr long OP_ERR_ORD_WRONG_INPUT = -300; // "입력값 오류"
    constexpr long OP_ERR_ORD_WRONG_ACCNO = -301; // "계좌비밀번호를 입력하십시오."
    constexpr long OP_ERR_OTHER_ACC_USE   = -302; // "타인계좌는 사용할 수 없습니다."
    constexpr long OP_ERR_MIS_2BILL_EXC   = -303; // "주문가격이 20억원을 초과합니다."
    constexpr long OP_ERR_MIS_5BILL_EXC   = -304; // "주문가격이 50억원을 초과합니다."
    constexpr long OP_ERR_MIS_1PER_EXC    = -305; // "주문수량이 총발행주수의 1%를 초과합니다."
    constexpr long OP_ERR_MIS_3PER_EXC    = -306; // "주문수량은 총발행주수의 3%를 초과할 수 없습니다."
    constexpr long OP_ERR_SEND_FAIL       = -307; // "주문전송실패"
    constexpr long OP_ERR_ORD_OVERFLOW    = -308; // "주문전송 과부하"
};

}

#endif /* SIBYL_SERVER_KIWOOM_KIWOOM_DATA_H_ */