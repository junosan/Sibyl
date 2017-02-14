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

#ifndef SIBYL_SERVER_KIWOOM_TR_H_
#define SIBYL_SERVER_KIWOOM_TR_H_

#include <map>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "KiwoomAPI.h"
#include "../../ostream_format.h"

namespace sibyl
{

// Synchronous operations : queries & order requests
class TR
{
public:
    // called by Kiwoom's constructor
    static void SetOrderBook(OrderBook<OrderKw, ItemKw> *pob_) { pob = pob_; }
    // called by Windows msg loop (initialization)
    static void SetAccNo    (CSTR &accno_) { accno = accno_; }

    enum class State { normal, carry, timeout, error };

    // called by Windows msg loop or NetServer thread (reqs)
    State Send(bool write = false);
        // this is blocking until a response is received or a timeout occurs
        // write  : true (initialization), false (compare)
        // return : normal | timeout | error

    // called by OpenAPI event thread (OnReceiveTrData)
    static void Receive(CSTR &name_, CSTR &code_, State state);
        // name   : OnReceiveTrData's sRQName
        // cnt    : # of idx to loop (GetRepeatCnt)
        // state  : normal | carry (sPrevNext == 2 for carry)

    TR() : writeOrderBook(false),
           end_bool(false),
           end_state(State::normal) {}
protected:
    static OrderBook<OrderKw, ItemKw> *pob;
    static CSTR& AccNo() { return accno; }
    
    std::atomic_bool writeOrderBook;

    // derived TR identification
    STR  TR_name;                     // set in derived TRs' constructor
    STR  TR_code;                     // set in derived TRs' constructor (if needed)
    STR  scrno;                       // set in derived TRs' constructor
    CSTR& Name () { return TR_name; } // use these outside of constructor
    CSTR& Code () { return TR_code; } // use these outside of constructor
    CSTR& ScrNo() { return scrno;   } // use these outside of constructor
    void Register  ();                // all derived TRs' constructor must call this
    void Deregister();                // all derived TRs' destructor  must call this
    
    virtual bool  AllowCarry()              = 0;
    virtual bool  AllowTimeout()            = 0;
    virtual long  SendOnce(State state)     = 0; // SetInputValue (if any) & CommRqData/SendOrder
    virtual void  RetrieveData(CSTR &code_) = 0;
    
    typedef KiwoomAPI K; // for brevity
private:
    static std::map<STR, TR*> map_name_TR;
    static STR accno;

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
                    scrno   = "9999";
                    Register  (); }
    ~TRRefPrice() { Deregister(); }
private:
    STR code;
    bool  AllowCarry()              override { return false;                                         }
    bool  AllowTimeout()            override { return false;                                         }
    long  SendOnce(State state)     override { K::SetInputValue(InputKey::code, code); //  
                                               return K::CommRqData(Name(), Code(), false, ScrNo()); }
    void  RetrieveData(CSTR &code_) override {
        STR data_code =           K::GetCommData(Name(), code_, 0, CommDataKey::code);
        INT data_refp = std::stoi(K::GetCommData(Name(), code_, 0, CommDataKey::refprice));
        
        std::lock_guard<std::recursive_mutex> lock(pob->items_mutex);
        auto it_itm = pob->items.find(data_code);
        if (it_itm != std::end(pob->items))
        {
            if (writeOrderBook == true)
            {
                it_itm->second->tb = std::array<PQ, idx::szTb>{};
                it_itm->second->tb[idx::ps1].p = data_refp;
            }
            std::cout << dispPrefix << "ref p " << fmt_code(data_code) << " " << fmt_price(data_refp) << std::endl;
        }
        else
            std::cerr << dispPrefix << Name() << "::RetrieveData: " << fmt_code(data_code) << " not found" << std::endl;
        code.clear(); // force SetCode for next call
    }
};

class TRAccBalance : public TR
{
public:
     TRAccBalance() { TR_name = "TRAccBalance";
                      TR_code = "OPW00001";
                      scrno   = "9998";
                      Register  (); }
    ~TRAccBalance() { Deregister(); }
private:
    bool  AllowCarry()              override { return false;                                         }
    bool  AllowTimeout()            override { return true;                                          }
    long  SendOnce(State state)     override { K::SetInputValue(InputKey::accno      , AccNo()); // 
                                               K::SetInputValue(InputKey::pin        , ""     ); // 
                                               K::SetInputValue(InputKey::pin_input_s, "00"   ); // 
                                               K::SetInputValue(InputKey::query_s    , "1"    ); // 
                                               return K::CommRqData(Name(), Code(), false, ScrNo()); }
    void  RetrieveData(CSTR &code_) override { 
        INT64 data_bal = static_cast<INT64>(std::stoll(K::GetCommData(Name(), code_, 0, CommDataKey::delayedbal)));
        
        std::lock_guard<std::recursive_mutex> lock(pob->items_mutex);
        if (writeOrderBook == true)
            pob->bal = data_bal;
        // align
            std::cout << dispPrefix << "bal i " << fmt_bal(pob->bal) << std::endl;
        if (pob->bal != data_bal)
            std::cout << dispPrefix << " != d " << fmt_bal(data_bal) << std::endl;
    }
};

class TRCntOList : public TR // cntO = cnt + sell_order_q 
{
public:
     TRCntOList() { TR_name = "TRCntOList";
                    TR_code = "OPW00018";
                    scrno   = "9997";
                    Register  (); }
    ~TRCntOList() { Deregister(); }
private:
    bool  AllowCarry()              override { return true;                                                          }
    bool  AllowTimeout()            override { return false;                                                         }
    long  SendOnce(State state)     override { K::SetInputValue(InputKey::accno      , AccNo()); // 
                                               K::SetInputValue(InputKey::pin        , ""     ); // 
                                               K::SetInputValue(InputKey::pin_input_s, "00"   ); // 
                                               K::SetInputValue(InputKey::query_s    , "2"    ); // 
                                               return K::CommRqData(Name(), Code(), state == State::carry, ScrNo()); }
    void  RetrieveData(CSTR &code_) override {
        long cnt = K::GetRepeatCnt(Name(), code_);
 
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
            STR data_code =           K::GetCommData(Name(), code_, idx, CommDataKey::code_g);
            INT data_cnto = std::stoi(K::GetCommData(Name(), code_, idx, CommDataKey::cnt_plus_so));
            
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
                    std::cout << dispPrefix << "inv i " << fmt_code(data_code) << " " << fmt_quant(internal_cnto) << std::endl;
                if (internal_cnto != data_cnto)
                    std::cout << dispPrefix << " != d " << fmt_code(data_code) << " " << fmt_quant(data_cnto)     << std::endl;
            }
            else
                std::cerr << dispPrefix << Name() << "::RetrieveData: " << fmt_code(data_code) << " not found" << std::endl;
        }
    }
};

class TROrdList : public TR
{
public:
     TROrdList() { TR_name = "TROrdList";
                   TR_code = "OPT10075";
                   scrno   = "9996";
                   Register  (); }
    ~TROrdList() { Deregister(); }
private:
    bool  AllowCarry()              override { return true;                                                          }
    bool  AllowTimeout()            override { return false;                                                         }
    long  SendOnce(State state)     override { K::SetInputValue(InputKey::accno    , AccNo()); // 
                                               K::SetInputValue(InputKey::reqstat_s, "1"    ); // 
                                               K::SetInputValue(InputKey::ordtype_s, "0"    ); // 
                                               return K::CommRqData(Name(), Code(), state == State::carry, ScrNo()); }
    void  RetrieveData(CSTR &code_) override { 
        long cnt = K::GetRepeatCnt(Name(), code_);
        
        std::lock_guard<std::recursive_mutex> lock(pob->items_mutex);
        if (writeOrderBook == true)
        {
            for (auto &code_pitm : pob->items)
                code_pitm.second->ord.clear();
        }
        for (long idx = 0; idx < cnt; idx++) // newest -> oldest; need to reverse ordering after receiving all carried data
        {
            OrderKw data_o;
            data_o.tck_orig = idx::tckN; // cannot track tck_orig from data
            data_o.q = std::stoi(K::GetCommData(Name(), code_, idx, CommDataKey::ordq));
            if (data_o.q > 0)
            {
                int type = std::stoi(K::GetCommData(Name(), code_, idx, CommDataKey::ordtype));
                data_o.type = (type == static_cast<int>(OrdType::buy ) ? OrdType::buy  :
                              (type == static_cast<int>(OrdType::sell) ? OrdType::sell : OrdType::null));
                if (data_o.type != OrdType::null)
                {
                    STR data_code = K::GetCommData(Name(), code_, idx, CommDataKey::code);
                    
                    auto it_itm = pob->items.find(data_code);
                    if (it_itm != std::end(pob->items))
                    {
                        data_o.ordno  =           K::GetCommData(Name(), code_, idx, CommDataKey::ordno);
                        data_o.p      = std::stoi(K::GetCommData(Name(), code_, idx, CommDataKey::ordp));
                        
                        auto &i = *it_itm->second; // reference as ItemKw
                        if (writeOrderBook == true)
                        {
                            if (data_o.type == OrdType::sell)
                            {
                                i.cnt -= data_o.q;
                                if (i.cnt < 0)
                                {
                                    std::cerr << dispPrefix << Name() << "::RetrieveData: " << fmt_code(data_code) << " cnt < 0 reached while subtracting sell orders" << std::endl;
                                    std::cerr << dispPrefix << Name() << "::RetrieveData: " << fmt_code(data_code) << " Setting cnt = 0" << std::endl;
                                    i.cnt = 0;
                                }
                            }
                            i.ord.insert(std::make_pair(data_o.p, data_o));
                        } 
                        
                        // find order matching data_o in i.ord
                        auto it = std::end(i.ord);
                        for (auto iO = std::begin(i.ord); iO != std::end(i.ord); iO++)
                            if (iO->second.ordno == data_o.ordno)
                            {
                                it = iO;
                                break;
                            }
                        
                        if (it != std::end(i.ord))
                            std::cout << dispPrefix << "ord i " << fmt_code(data_code) << " " << it->second << std::endl;
                        else
                            std::cout << dispPrefix << "ord i " << fmt_code(data_code) << " missing" << std::endl;
                        if (it == std::end(i.ord) || it->second.type != data_o.type ||
                                                     it->second.p    != data_o.p    ||
                                                     it->second.q    != data_o.q     )
                            std::cout << dispPrefix << " != d " << fmt_code(data_code) << " " << data_o << std::endl;
                    }
                    else
                        std::cerr << dispPrefix << Name() << "::RetrieveData: " << fmt_code(data_code) << " not found" << std::endl;
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
        std::lock_guard<std::recursive_mutex> lock(pob->items_mutex);
        auto it_itm = pob->items.find(code);
        if (it_itm != std::end(pob->items))
        {
            // use scrno 100 lower than item's (to avoid potential conflict with SetRealReg)
            scrno = std::to_string(std::stoi(it_itm->second->srcno) - 100);
        }
        else
        {
            std::cerr << dispPrefix << Name() << "::SetReq: " << fmt_code(code) << " not found" << std::endl;
            scrno = "9995";
        }
    }
     TRReqOrder() : type(ReqType::null)
                  { TR_name = "TRReqOrder";
                    Register  (); }
    ~TRReqOrder() { Deregister(); }
private:
    ReqType type;    STR code;    PQ pq;    STR ordno_o;
    bool  AllowCarry()              override { return false;                                 }
    bool  AllowTimeout()            override { return true;                                  }
    long  SendOnce(State state)     override { // debug_msg("[ReqOrder] " << type << " " << fmt_code(code) << " "
                                               //        << fmt_price(pq.p) << " " << fmt_quant(pq.q) << " " << fmt_ordno(ordno_o));
                                               return K::SendOrder(Name(), ScrNo(), AccNo(),
                                                                   type, code, pq, ordno_o); }
    void  RetrieveData(CSTR &code_) override { type = ReqType::null; /* force SetReq */      }
};

class TRIndex : public TR
{
public:
     TRIndex() { TR_name = "TRIndex";
                 TR_code = "OPT20001";
                 scrno   = "9994";
                 Register  (); }
    ~TRIndex() { Deregister(); }
private:
    bool  AllowCarry()              override { return false;                                         }
    bool  AllowTimeout()            override { return false;                                         }
    long  SendOnce(State state)     override { K::SetInputValue(InputKey::market_s   , "0"  );
                                               K::SetInputValue(InputKey::sector_code, "201"); // KOSPI200 index
                                               // (001: KOSPI index, 201: KOSPI200 index, etc.)
                                               return K::CommRqData(Name(), Code(), false, ScrNo()); }
    void  RetrieveData(CSTR &code_) override {                                                       }
};

}

#endif /* SIBYL_SERVER_KIWOOM_TR_H_ */