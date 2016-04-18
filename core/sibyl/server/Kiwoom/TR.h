#ifndef SIBYL_SERVER_KIWOOM_TR_H_
#define SIBYL_SERVER_KIWOOM_TR_H_

#include <map>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "KiwoomFunction.h"

namespace sibyl
{

// Designed set of KiwoomFunction operations to implement TR
// (i.e., synchronous queries & order requests)
class TR : public KiwoomFunction
{
public:
    enum class State { normal, carry, timeout };

    // called by Windows msg loop or NetServer thread (reqs)
    State Send(bool write = false);
        // this is blocking until a response is received or a timeout occurs
        // write  : true (initialization), false (compare)
        // return : normal | timeout

    // called by OpenAPI event thread (OnReceiveTrData)
    static void Receive(CSTR &name, long cnt, State state);
        // name   : OnReceiveTrData's sRQName
        // cnt    : # of idx to loop (GetRepeatCnt)
        // state  : normal | carry (sPrevNext == 2 for carry)

    TR() : writeOrderBook(false),
           end_bool(false),
           end_state(State::normal) {}
protected:
    std::atomic_bool writeOrderBook;

    // derived TR identification
    STR  TR_name;                    // set in derived TRs' constructor
    STR  TR_code;                    // set in derived TRs' constructor (if needed)
    CSTR& Name() { return TR_name; } // use these outside of constructor
    CSTR& Code() { return TR_code; } // use these outside of constructor
    void Register  ();               // all derived TRs' constructor must call this
    void Deregister();               // all derived TRs' destructor  must call this
    
    virtual bool  AllowCarry()           = 0;
    virtual bool  AllowTimeout()         = 0;
    virtual long  SendOnce(State state)  = 0; // SetInputValue (if any) & CommRqData/SendOrder
    virtual void  RetrieveData(long cnt) = 0;
private:
    static std::map<STR, TR*> map_name_TR;

    constexpr static int kNoTimeout     = -1;
    constexpr static int t_timeout      = 5000;
    constexpr static int t_waitOverflow = 1000 / kTimeRates::reqPerSec; // 200 ms
    
    // called by Windows msg loop or NetServer thread (reqs)
    void Init();               // regulate TR rate per sec
    State Wait(int t_timeout); // milliseconds; negative values are reserved
    // called by OpenAPI event thread (OnReceiveTrData)
    void End(State state);     // state : normal | carry
    
    std::condition_variable cv;
    std::mutex              cv_mutex;
    std::atomic_bool        end_bool;
    std::atomic<State>      end_state;
};

class TRRefPrice : public TR
{
public:
    void SetCode(CSTR &code_) { code = code_; }
     TRRefPrice() { TR_name = "TRRefPrice";
                    TR_code = "OPT10001";
                    Register  (); }
    ~TRRefPrice() { Deregister(); }
private:
    STR code;
    bool  AllowCarry()           override { return false;                             }
    bool  AllowTimeout()         override { return false;                             }
    long  SendOnce(State state)  override { SetInputValue(InputKey::code, code); // 종목코드 
                                            return CommRqData(Name(), Code(), false); }
    void  RetrieveData(long cnt) override {
        STR data_code =           GetCommData(Name(), Code(), 0, CommDataKey::code);
        INT data_refp = std::stoi(GetCommData(Name(), Code(), 0, CommDataKey::refprice));
        
        std::lock_guard<std::recursive_mutex> lock(pob->items_mutex);
        auto it_itm = pob->items.find(data_code);
        if (it_itm != std::end(pob->items))
        {
            if (writeOrderBook == true)
            {
                it_itm->second->tb = std::array<PQ, idx::szTb>{};
                it_itm->second->tb[idx::ps1].p = data_refp;
            }
            std::cout << dispPrefix << "refp {" << data_code << "} " << data_refp << std::endl;
        }
        else
            std::cerr << dispPrefix << Name() << "::RetrieveData: {" << data_code << "} not found" << std::endl;
        code.clear(); // force SetCode for next call
    }
};

class TRAccBalance : public TR
{
public:
     TRAccBalance() { TR_name = "TRAccBalance";
                      TR_code = "OPW00001";
                      Register  (); }
    ~TRAccBalance() { Deregister(); }
private:
    bool  AllowCarry()           override { return false;                               }
    bool  AllowTimeout()         override { return true;                                }
    long  SendOnce(State state)  override { SetInputValue(InputKey::accno      , ""  ); // 계좌번호
                                            SetInputValue(InputKey::pin        , ""  ); // 비밀번호
                                            SetInputValue(InputKey::pin_input_s, "00"); // 비밀번호입력매체구분
                                            SetInputValue(InputKey::query_s    , "1" ); // 조회구분
                                            return CommRqData(Name(), Code(), false);   }
    void  RetrieveData(long cnt) override { 
        INT64 data_bal = static_cast<INT64>(std::stoll(GetCommData(Name(), Code(), 0, CommDataKey::delayedbal)));
        
        std::lock_guard<std::recursive_mutex> lock(pob->items_mutex);
        if (writeOrderBook == true)
            pob->bal = data_bal;
        // align
            std::cout << dispPrefix << "bal i " << pob->bal << std::endl;
        if (pob->bal != data_bal)
            std::cout << dispPrefix << " != d " << data_bal << std::endl;
    }
};

class TRCntOList : public TR // cntO = cnt + sell_order_q 
{
public:
     TRCntOList() { TR_name = "TRCntOList";
                    TR_code = "OPW00018";
                    Register  (); }
    ~TRCntOList() { Deregister(); }
private:
    bool  AllowCarry()           override { return true;                                              }
    bool  AllowTimeout()         override { return false;                                             }
    long  SendOnce(State state)  override { SetInputValue(InputKey::accno      , ""  ); // 계좌번호
                                            SetInputValue(InputKey::pin        , ""  ); // 비밀번호
                                            SetInputValue(InputKey::pin_input_s, "00"); // 비밀번호입력매체구분
                                            SetInputValue(InputKey::query_s    , "2" ); // 조회구분
                                            return CommRqData(Name(), Code(), state == State::carry); }
    void  RetrieveData(long cnt) override { 
        std::lock_guard<std::recursive_mutex> lock(pob->items_mutex);
        if (writeOrderBook == true)
        {
            for (auto &code_pitm : pob->items)
            {
                code_pitm.second->cnt = 0;
                code_pitm.second->ord.clear();
            }
        }
        for (long idx = 0; idx < cnt; idx++)
        {
            STR data_code =           GetCommData(Name(), Code(), idx, CommDataKey::code_g);
            INT data_cnto = std::stoi(GetCommData(Name(), Code(), idx, CommDataKey::cnt_plus_so));
            
            auto it_itm = pob->items.find(data_code);
            if (it_itm != std::end(pob->items))
            {
                auto &i = *it_itm->second;
                if (writeOrderBook == true)
                    i.cnt = data_cnto; // need to subtract actual sell_order_q later
                    
                // count current cnt + sell_order_q
                INT internal_cnto = i.cnt;
                for (const auto &price_order : i.ord) {
                    if (price_order.second.type == OrdType::sell)
                        internal_cnto += price_order.second.q;
                }
                // align
                    std::cout << dispPrefix << "cnto i {" << data_code << "} (" << internal_cnto << ")" << std::endl;
                if (internal_cnto != data_cnto)
                    std::cout << dispPrefix << "  != d {" << data_code << "} (" << data_cnto     << ")" << std::endl;
            }
            else
                std::cerr << dispPrefix << Name() << "::RetrieveData: {" << data_code << "} not found" << std::endl;
        }
    }
};

class TROrdList : public TR
{
public:
     TROrdList() { TR_name = "TROrdList";
                   TR_code = "OPT10075";
                   Register  (); }
    ~TROrdList() { Deregister(); }
private:
    bool  AllowCarry()           override { return true;                                              }
    bool  AllowTimeout()         override { return false;                                             }
    long  SendOnce(State state)  override { SetInputValue(InputKey::accno    , "" ); // 계좌번호
                                            SetInputValue(InputKey::reqstat_s, "1"); // 체결구분
                                            SetInputValue(InputKey::ordtype_s, "0"); // 매매구분
                                            return CommRqData(Name(), Code(), state == State::carry); }
    void  RetrieveData(long cnt) override { 
        std::lock_guard<std::recursive_mutex> lock(pob->items_mutex);
        if (writeOrderBook == true)
        {
            for (auto &code_pitm : pob->items)
                code_pitm.second->ord.clear();
        }
        for (long idx = 0; idx < cnt; idx++) // newest->oldest; need to reverse ordering after receiving all carried data
        {
            OrderKw data_o;
            data_o.tck_orig = idx::tckN; // cannot track tck_orig from data
            data_o.q = std::stoi(GetCommData(Name(), Code(), idx, CommDataKey::ordq));
            if (data_o.q > 0)
            {
                int type = std::stoi(GetCommData(Name(), Code(), idx, CommDataKey::ordtype));
                data_o.type = (type == static_cast<int>(OrdType::buy ) ? OrdType::buy  :
                              (type == static_cast<int>(OrdType::sell) ? OrdType::sell : OrdType::null));
                if (data_o.type != OrdType::null)
                {
                    STR data_code = GetCommData(Name(), Code(), idx, CommDataKey::code);
                    
                    auto it_itm = pob->items.find(data_code);
                    if (it_itm != std::end(pob->items))
                    {
                        data_o.ordno  =           GetCommData(Name(), Code(), idx, CommDataKey::ordno);
                        data_o.p      = std::stoi(GetCommData(Name(), Code(), idx, CommDataKey::ordp));
                        
                        auto &i = *it_itm->second;
                        if (writeOrderBook == true)
                        {
                            if (data_o.type == OrdType::sell)
                            {
                                i.cnt -= data_o.q;
                                if (i.cnt < 0)
                                {
                                    std::cerr << dispPrefix << Name() << "::RetrieveData: {" << data_code << "} cnt < 0 reached while subtracting sell orders" << std::endl;
                                    std::cerr << dispPrefix << Name() << "::RetrieveData: {" << data_code << "} Setting cnt = 0" << std::endl;
                                    i.cnt = 0;
                                }
                            }
                            i.ord.insert(std::make_pair(data_o.p, data_o));
                        } 
                        
                        // find order matching data_o in i.ord
                        auto it = std::end(i.ord);
                        // INT internal_cnto = i.cnt;
                        // for (const auto &price_order : i.ord) {
                        //     if (price_order.second.type == OrdType::sell)
                        //         internal_cnto += price_order.second.q;
                        // }
                        // align
                        //     std::cout << dispPrefix << "cnto i {" << data_code << "} (" << internal_cnto << ")" << std::endl;
                        // if (internal_cnto != data_cnto)
                        //     std::cout << dispPrefix << "  != d {" << data_code << "} (" << data_cnto     << ")" << std::endl;
                    }
                    else
                        std::cerr << dispPrefix << Name() << "::RetrieveData: {" << data_code << "} not found" << std::endl;
                }
            }
        }
    }
};

class TRReqOrder : public TR
{
public:
    void SetReq(ReqType type_, CSTR &code_, PQ pq_, CSTR &ordno_o_) {
        type = type_; code = code_; pq = pq_; ordno_o = ordno_o_;
    }
     TRReqOrder() : type(ReqType::null)
                  { TR_name = "TRReqOrder";
                    Register  (); }
    ~TRReqOrder() { Deregister(); }
private:
    ReqType type;    STR code;    PQ pq;    STR ordno_o;
    bool  AllowCarry()           override { return false;                                      }
    bool  AllowTimeout()         override { return true;                                       }
    long  SendOnce(State state)  override { return SendOrder(Name(), type, code, pq, ordno_o); }
    void  RetrieveData(long cnt) override { type = ReqType::null; /* force SetReq */           }
};

}

#endif /* SIBYL_SERVER_KIWOOM_TR_H_ */