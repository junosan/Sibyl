/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include "Kiwoom.h"

#include <thread>
#include <fstream>
#include <iomanip>

#include "../../util/Clock.h"
#include "../../ostream_format.h"
#include "../../util/Config.h"

namespace sibyl
{

void Kiwoom::SetStateFile(CSTR &filename)
{
    {
        std::ofstream ofs(filename);
        verify(ofs.is_open() == true);
    }
    stateFileName = filename;
}

void Kiwoom::ReadConfigFiles(CSTR &config, CSTR &codelist)
{
    orderbook.items.clear();
    
    Config cfg(config), list(codelist);
    
    // this part can be revised using polymorphic lambdas (C++14)
    int val;
    
    auto &ssUSE_KOSPI = cfg.Get("USE_KOSPI");
    ssUSE_KOSPI >> val;
    if (ssUSE_KOSPI.fail() == false && val != 0) // USE_KOSPI == true
    {
        auto &ss = list.Get("KRX_CODE");
        // int n = 0; // for debugging
        for (STR code; std::getline(ss, code, ';');)
        {
            auto it_success = orderbook.items.insert(std::make_pair(code, std::unique_ptr<ItemKw>(new KOSPI<ItemKw>)));
            if (it_success.second == false) // new pointer is deleted automatically in destructor
                std::cerr << dispPrefix << "ReadConfig: Nonunique KOSPI code " << fmt_code(code) << std::endl;
            // if (++n == 5) break; // for debugging
        }
    }
    
    auto &ssUSE_ETF = cfg.Get("USE_ETF");
    ssUSE_ETF >> val;
    if (ssUSE_ETF.fail() == false && val != 0) // USE_ETF == true
    {
        auto &ss = list.Get("ETF_CODE");
        for (STR code; std::getline(ss, code, ';');)
        {
            auto it_success = orderbook.items.insert(std::make_pair(code, std::unique_ptr<ItemKw>(new ETF<ItemKw>)));
            if (it_success.second == false) // new pointer is deleted automatically in destructor
                std::cerr << dispPrefix << "ReadConfig: Nonunique ETF code " << fmt_code(code) << std::endl;
        }
    }
    
    std::cerr << dispPrefix << "ReadConfig: " << orderbook.items.size() << " securities read from config files" << std::endl;
}

bool Kiwoom::Launch()
{
    // Retrieve reference price and set it at tb[idx::ps1]
    std::cerr << dispPrefix << "Launch: Querying daily reference price" << std::endl;
    
    for (auto it_itm = std::begin(orderbook.items); it_itm != std::end(orderbook.items);)
    {
        trRefPrice.SetCode(it_itm->first);
        
        if (TR::State::error == trRefPrice.Send(true))
            return false;
            
        if (it_itm->second->tb[idx::ps1].p > 0)
            it_itm++;
        else // remove invalid code
            it_itm = orderbook.items.erase(it_itm);
    }
    
    std::cerr << dispPrefix << "Launch: " << orderbook.items.size() << " valid securities" << std::endl;
    
    // Retrieve balance 
    std::cerr << dispPrefix << "Launch: Querying d+2 balance" << std::endl;
    
    TR::State state = trAccBalance.Send(true);
    if (state == TR::State::error || state == TR::State::timeout)
        return false;
    if (orderbook.bal <= 0) {
        std::cerr << dispPrefix << "Launch: [Fail] Nonpositive balance" << std::endl;
        return false;
    }
    
    // Retrieve cnt + sell_order_q
    std::cerr << dispPrefix << "Launch: Querying inventory (idle + sell order)" << std::endl;
    
    if (TR::State::error == trCntOList.Send(true))
        return false;
    
    // Retrieve ord (newest -> oldest)
    std::cerr << dispPrefix << "Launch: Querying previously placed orders" << std::endl;
    
    if (TR::State::error == trOrdList.Send(true))
        return false;
    
    // Reverse ord ordering
    for (auto &code_pItem : orderbook.items)
    {
        std::multimap<INT, OrderKw> reverse;
        for (auto rit_ord = code_pItem.second->ord.rbegin(); rit_ord != code_pItem.second->ord.rend(); rit_ord++)
            reverse.insert(std::make_pair(rit_ord->first, rit_ord->second));
        code_pItem.second->ord.swap(reverse);
    }
    
    // Build FID list
    std::vector<long> vFID = { kFID::tr_t,
                               kFID::tr_p,
                               kFID::tr_q,
                               kFID::tr_ps1,
                               kFID::tr_pb1,
                               kFID::tb_t };
                               
    auto AddAllTck = [&](long base) {
        for (long i = 0; i < (long) idx::tckN; i++) vFID.push_back(base + i);
    };
    
    AddAllTck(kFID::tb_ps1);
    AddAllTck(kFID::tb_pb1);
    AddAllTck(kFID::tb_qs1);
    AddAllTck(kFID::tb_qb1);
    
    STR FID_KOSPI;
    for (auto FID : vFID) FID_KOSPI += std::to_string(FID) + ';';
    STR FID_ETF = FID_KOSPI + std::to_string(kFID::nav_NAV);
    if (FID_KOSPI.back() == ';') FID_KOSPI.pop_back();
    
    // Attach scrno; use a different scrno for every 100 items
    // Currently using 9999~9995 for TR's
    auto AllotScrNo = [&](SecType type, int scrno, std::vector<std::pair<int, STR>> &scrno_codes) {
        STR codes;
        int n = 0;
        for (auto &code_pItem : orderbook.items) {
            if (code_pItem.second->Type() == type) {
                codes += code_pItem.first + ';';
                code_pItem.second->srcno = std::to_string(scrno); // used later for placing orders (at scrno - 100)
                if (++n == 100) {
                    if (codes.back() == ';') codes.pop_back();
                    scrno_codes.push_back(std::make_pair(scrno--, codes));
                    codes.clear();
                    n = 0;
                }
            }
        }
        if (n > 0) {
            if (codes.back() == ';') codes.pop_back();
            scrno_codes.push_back(std::make_pair(scrno, codes));
        }
    };
    
    std::vector<std::pair<int, STR>> scrno_codes_KOSPI;
    std::vector<std::pair<int, STR>> scrno_codes_ETF;
    
    AllotScrNo(SecType::KOSPI, 9990, scrno_codes_KOSPI); // 9990, 9989, ...
    AllotScrNo(SecType::ETF  , 9980, scrno_codes_ETF  ); // 9980, 9979, ...

    // SetRealReg
    long retsum = 0;
    for (const auto &scrno_codes : scrno_codes_KOSPI)
    {
        retsum += K::SetRealReg(std::to_string(scrno_codes.first), scrno_codes.second, FID_KOSPI);
        // debug_msg("SetRealReg " << scrno_codes.second << " " << FID_KOSPI);
    }
    for (const auto &scrno_codes : scrno_codes_ETF  )
        retsum += K::SetRealReg(std::to_string(scrno_codes.first), scrno_codes.second, FID_ETF  );
    
    if (retsum != 0)
    {
        std::cerr << dispPrefix << "Launch: SetRealReg failed" << std::endl;
        return false;
    }
    
    return true;
}

void Kiwoom::OnExit()
{
    std::cerr << dispPrefix << "OnExit: Comparing d+2 balance" << std::endl;
    trAccBalance.Send(false);
    
    std::cerr << dispPrefix << "OnExit: Comparing inventory (idle + sell order)" << std::endl;
    trCntOList.Send(false);
    
    std::cerr << dispPrefix << "OnExit: Comparing previously placed orders" << std::endl;
    trOrdList.Send(false);
}

int Kiwoom::AdvanceTick()
{
    int timeTarget = GetOrderBookTime() + kTimeRates::secPerTick; // s from 09:00:00
    int t_target = (timeTarget - timeOffset) * 1000; // ms from 00:00:00
    int t_wait   = t_target - (clock.Now() + t_data_minus_local); // synchronize to data time
    
    if (t_wait <= -kTimeRates::secPerTick * 1000)
        skipTick = true; // skip time tick if more than one tick behind
    else
    {
        if (t_wait > 0) std::this_thread::sleep_for(std::chrono::milliseconds(t_wait));
        skipTick = false;
        ResetInterrupt();
    }
    
    SetOrderBookTime(timeTarget);    
    
    return (GetOrderBookTime() < kTimeBounds::end ? 0 : -1);
}

CSTR& Kiwoom::BuildMsgOut()
{
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    
    for (const auto &code_pItem : orderbook.items)
    {
        auto &i = *code_pItem.second;
        
        // requantize
        i.tbr = i.tb;
        if (GetOrderBookTime() > 0) i.Requantize(i.trPs1, i.trPb1);
        else                        i.Requantize();
        
        // calc pr, qr
        if (i.sumQ > 0)              i.pr = (FLOAT) i.sumPQ / i.sumQ;
        if (GetOrderBookTime() <= 0) i.pr = (FLOAT) i.Ps0();
        i.qr = i.sumQ;
        i.sumQ = i.sumPQ = 0;
    }
    
    orderbook.UpdateRefInitBal();
    
    if (verbose == true && stateFileName.empty() == false) WriteState();
    
    return orderbook.BuildMsgOut(false);
}

void Kiwoom::WriteState()
{
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    
    std::ofstream ofs(stateFileName, std::ios::trunc);
    
    const int  nItemPerLine = 2;
    const char itemSpacer[] = " ";
    static char buf[1 << 8];
    
    ofs << "[t=" << std::setw(5) << GetOrderBookTime() << "]\n";
    ofs << "bal " << fmt_bal(orderbook.bal) << '\n';
    auto evl = orderbook.Evaluate();
    ofs << "evl " << fmt_bal(evl.evalTot);
    sprintf(buf, " (r%+.2f%%) (s%+.2f%%)\n", (orderbook.GetProfitRate(true) - 1.0) * 100.0, (orderbook.GetProfitRate(false) - 1.0) * 100.0);
    ofs << buf;
    
    ofs << "cnt\n";
    
    int nItemCur = 0;
    for (const auto &code_pItem : orderbook.items)
    {
        const auto &i = *code_pItem.second;
        if (i.cnt > 0)
        {
            sprintf(buf, "      {%s} %8d (%6d)", code_pItem.first.c_str(), i.Ps0(), i.cnt);
            ofs << buf;
            if (nItemPerLine == ++nItemCur)
            {
                ofs << "\n";
                nItemCur = 0;
            }
            else
                ofs << itemSpacer;
        }
    }
    if (nItemCur != 0) ofs << "\n";	
    
    ofs << "ord\n";
    
    auto ListOrder = [&](const OrdType &type) {
        nItemCur = 0;
        for (const auto &code_pItem : orderbook.items)
        {
            const auto &i = *code_pItem.second;
            for (const auto &price_Order : i.ord)
            {
                const auto &o = price_Order.second;
                if (o.type == type && o.q > 0)
                {
                    int tck = i.P2Tck(o.p, o.type); // 0-based tick
                    if (tck == idx::tckN) tck = 98; // display as 99 if not found
                    sprintf(buf, "[%s%2d] {%s} %8d (%6d)", (type == OrdType::buy ? "b" : "s"), tck + 1, code_pItem.first.c_str(), o.p, o.q);
                    ofs << buf;
                    if (nItemPerLine == ++nItemCur)
                    {
                        ofs << "\n";
                        nItemCur = 0;
                    }
                    else
                        ofs << itemSpacer;
                }
            }
        }
        if (nItemCur != 0)  ofs << "\n";
    };
    
    ListOrder(OrdType::buy) ;
    ListOrder(OrdType::sell);
    ofs << std::endl;
}

void Kiwoom::ReceiveMarketTr(CSTR &code)
{
    INT p     = std::abs(std::stoi(K::GetCommRealData(code, kFID::tr_p  )));
    INT q     = std::abs(std::stoi(K::GetCommRealData(code, kFID::tr_q  )));
    INT trPs1 = std::abs(std::stoi(K::GetCommRealData(code, kFID::tr_ps1)));
    INT trPb1 = std::abs(std::stoi(K::GetCommRealData(code, kFID::tr_pb1)));
    
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    auto it_itm = FindItem(code);
    if (it_itm != std::end(orderbook.items))
    {
        // ignore trade data before 09:00:00 as their ps1/pb1 values are invalid
        if (GetOrderBookTime() < 0)
            p = q = trPs1 = trPb1 = 0;
        
        auto &i = *it_itm->second;
        i.sumQ  += (INT64) q;
        i.sumPQ += (INT64) p * q;
        i.trPs1  = trPs1;
        i.trPb1  = trPb1;
    }
}

void Kiwoom::ReceiveMarketTb(CSTR &code)
{
    STR time = K::GetCommRealData(code, kFID::tb_t);
    std::array<PQ, idx::szTb> tb;
    for (int i = 0; i < idx::tckN; i++)
    {
        tb[(std::size_t) (idx::ps1 - i)].p = std::abs(std::stoi(K::GetCommRealData(code, kFID::tb_ps1 + (long) i)));
        tb[(std::size_t) (idx::ps1 - i)].q = std::abs(std::stoi(K::GetCommRealData(code, kFID::tb_qs1 + (long) i)));
        tb[(std::size_t) (idx::pb1 + i)].p = std::abs(std::stoi(K::GetCommRealData(code, kFID::tb_pb1 + (long) i)));
        tb[(std::size_t) (idx::pb1 + i)].q = std::abs(std::stoi(K::GetCommRealData(code, kFID::tb_qb1 + (long) i)));
    }
    
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    static STR lastTime;
    if (lastTime != time) // update at new second (i.e., closest to HHMMSS.000)
    {
        int t_data = Clock::HHMMSS_to_ms(time);
        t_data_minus_local = t_data - clock.Now();
        if (t_data / 1000 + timeOffset >= GetOrderBookTime() + kTimeRates::secPerTick)
            InterruptExec();
        lastTime = time;
    }
    
    auto it_itm = FindItem(code);
    if (it_itm != std::end(orderbook.items))
        it_itm->second->tb = tb;
}

void Kiwoom::ReceiveMarketNAV(CSTR &code)
{
    INT p     = std::abs(std::stoi(K::GetCommRealData(code, kFID::tr_p   )));
    FLOAT nav = std::abs(std::stof(K::GetCommRealData(code, kFID::nav_NAV)));
    
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    auto it_itm = FindItem(code);
    if (it_itm != std::end(orderbook.items))
    {
        auto &i = *it_itm->second;
        if (i.Type() == SecType::ETF) 
            static_cast<ETF<ItemKw>&>(i).devNAV = (FLOAT) (((double) nav / p - 1.0) * 100.0);
        else
            std::cerr << dispPrefix << "ApplyRealtimeNAV: " << fmt_code(code) << " is not ETF" << std::endl;
    }
}

void Kiwoom::ReceiveOrdEvent()
{
    ReqStat reqStat = static_cast<ReqStat>(std::stoi(K::GetChejanData(kFID::reqstat)));
    
    if (reqStat == ReqStat::confirmed)
    {
        // debug_msg("[OrdEvent] ReqStat::confirmed received");

        // TEMPORARY //
        ReqType reqType = static_cast<ReqType>(std::stol(K::GetChejanData(kFID::reqtype)));
        INT ordq  = std::stoi(K::GetChejanData(kFID::ordq ));
        if ((reqType == ReqType::cb || reqType == ReqType::cs) && ordq > 0) {
            STR code  =           K::GetChejanData(kFID::code );
            INT ordq_i = std::stoi(K::GetChejanData(kFID::ordq_i));
            debug_msg("[IncompleteCancel] " << fmt_code(code) << " " << reqType << " " << fmt_quant(ordq_i) << " orderd but "
                      << fmt_quant(ordq) << " left");
        }
        // TEMPORARY //

        return;
    }

    INT delta_p(0), delta_q(0);
    if (reqStat == ReqStat::traded) // kFID::delta_* returns empty string otherwise
    {
        auto &s_p = K::GetChejanData(kFID::delta_p);
        if (s_p.empty() == true) return; // part of cancel receive-confirm-trade (confirm-trade holds no new information)
        delta_p = std::stoi(s_p);
        
        auto &s_q = K::GetChejanData(kFID::delta_q);
        if (s_q.empty() == true) return; // part of cancel receive-confirm-trade (confirm-trade holds no new information)
        delta_q = std::stoi(s_q);
    }
    
    ReqType reqType = static_cast<ReqType>(std::stol(K::GetChejanData(kFID::reqtype)));
    
    STR code  =           K::GetChejanData(kFID::code );
    STR ordno =           K::GetChejanData(kFID::ordno);
    INT ordp  = std::stoi(K::GetChejanData(kFID::ordp ));
    INT ordq  = std::stoi(K::GetChejanData(kFID::ordq ));
    
    STR ordno_o;
    if (reqStat == ReqStat::received &&
        (reqType == ReqType::cb || reqType == ReqType::cs || reqType == ReqType::mb || reqType == ReqType::ms))
    {
        ordno_o = K::GetChejanData(kFID::ordno_o);
    }
    
    // debug_msg("[     OrdEvent] " << fmt_code(code) << " " << static_cast<int>(reqStat) << " " << reqType << " "
    //           << fmt_ordno(ordno) << " " << fmt_price(ordp) << " " << fmt_quant(ordq) << " " << fmt_ordno(ordno_o) << "(o) "
    //           << "delta " << fmt_price(delta_p) << " " << fmt_quant(delta_q));
    
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    
    // pre-filter non-existent item
    auto it_itm = FindItem(code);
    if (it_itm == std::end(orderbook.items))
    {
        std::cerr << dispPrefix << "ApplyOrdEvent: " << fmt_code(code) << " not found" << std::endl;
        return;
    }

    auto OrderExists = [&](const it_ord_t<OrderKw> &it_ord) {
        return it_ord != std::end(it_itm->second->ord);
    };

    if (reqStat == ReqStat::received)
    {
        if (reqType != ReqType::cb && reqType != ReqType::cs)
        {
            if (ordq > 0) // avoid re-receiving emptied order after c | m
            {
                // debug_msg("[OrdEvent] " << fmt_ordno(ordno) << " b | s | mb | ms received");
                auto it_ord = FindOrder(it_itm, ordp, ordno, true);
                if (OrderExists(it_ord) == false)
                { 
                    // debug_msg("[OrdEvent] " << fmt_ordno(ordno) << " going to be inserted");
                    
                    if (reqType == ReqType::mb || reqType == ReqType::ms) // trim original order
                    {
                        auto it_ord_o = FindOrder(it_itm, 0, ordno_o);
                        if (OrderExists(it_ord_o) == true)
                            orderbook.ApplyCancel(it_itm, it_ord_o, ordq);
                        else
                            std::cerr << dispPrefix << "ApplyOrdEvent: mb | ms event received on a non-existent order" << std::endl;
                    }
                    
                    OrderKw o;
                    o.type  = (reqType == ReqType::b || reqType == ReqType::mb ? OrdType::buy : OrdType::sell);
                    o.p     = ordp;
                    o.q     = ordq;
                    o.ordno = ordno;
                    orderbook.ApplyInsert(it_itm, o); 
                }
            }
        }
        else
        {
            auto it_ord_o = FindOrder(it_itm, 0, ordno_o); // OpenAPI generates ordp = 0 for cancels
            if (OrderExists(it_ord_o) == true)
                orderbook.ApplyCancel(it_itm, it_ord_o, ordq);
            else
                std::cerr << dispPrefix << "ApplyOrdEvent: cb | cs event received on a non-existent order" << std::endl;
        }
    }
    else if (reqStat == ReqStat::traded)
    {
        auto it_ord = FindOrder(it_itm, ordp, ordno);
        if (OrderExists(it_ord) == true)
        {
            orderbook.ApplyTrade(it_itm, it_ord, PQ(delta_p, delta_q));
            if (it_ord->second.q != ordq)
                std::cerr << dispPrefix << "ApplyOrdEvent: order.q " << fmt_quant(it_ord->second.q) << " != event.q " << fmt_quant(ordq) << " after trade" << std::endl;
        }
        else
            std::cerr << dispPrefix << "ApplyOrdEvent: Trade event received on a non-existent order" << std::endl;
    }
}

void Kiwoom::ReceiveCntEvent()
{
    STR code =           K::GetChejanData(kFID::code);
    INT cnt  = std::stoi(K::GetChejanData(kFID::cnt));
    
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    auto it_itm = FindItem(code);
    if (it_itm != std::end(orderbook.items))
    {
        auto &i = *it_itm->second;
        if (i.cnt != cnt)
        {
            // Note: this may trigger naturally if a req is sent by me & received by server
            //       in between a pair of OrdEvent & CntEvent (safe to ignore)
            std::cerr << dispPrefix << "ApplyCntEvent: " << fmt_code(code) << " item.cnt " << fmt_quant(i.cnt) << " != event.cnt " << fmt_quant(cnt) << std::endl;
        }
    }
}

#include "../../util/toggle_win32_min_max.h"

int Kiwoom::ExecuteNamedReq(NamedReq<OrderKw, ItemKw> req)
{
    // debug_msg("[Kiwoom] Entered ExecuteNamedReq");
    
    STR code, ordno_o;
    
    {
        std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
        
        code = req.iItems->first;
        const auto &i = *req.iItems->second;
        
        // correct req based on most up-to-date orderbook state
        // also see OrderBook::AllotReq
        if (req.type == ReqType::b || req.type == ReqType::mb)
            req.p = std::min(req.p, i.Pb0());
        if (req.type == ReqType::s || req.type == ReqType::ms)
            req.p = std::max(req.p, i.Ps0());
        
        if      (req.type == ReqType::b)
            req.q = std::min(req.q, i.MaxBuyQ(orderbook.bal, req.p));
        else if (req.type == ReqType::s)
            req.q = std::min(req.q, i.cnt);
        else if (req.type == ReqType::cb || req.type == ReqType::cs || req.type == ReqType::mb || req.type == ReqType::ms)
        {
            ordno_o = req.iOrd->second.ordno;
            if (req.type == ReqType::mb && req.p > req.iOrd->second.p)
                req.q = std::min(req.q, i.MaxBuyQ(orderbook.bal, req.p - req.iOrd->second.p));
            req.q = std::min(req.q, req.iOrd->second.q);
        }
    } // mutex unlocked here
    
    if (req.q > 0)
    {
        trReqOrder.SetReq(req.type, code, PQ(req.p, req.q), ordno_o);
        trReqOrder.Send();
        // debug_msg("[Kiwoom] Exited trReqOrder.Send");
    }
    
    return 0;
}

#include "../../util/toggle_win32_min_max.h"

it_itm_t<ItemKw> Kiwoom::FindItem(CSTR &code)
{
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    auto it_itm = orderbook.items.find(code);
    
    // for some reason, non-existent items are received quite frequently; suppress this warning 
    // if (it_itm == std::end(orderbook.items))
    //     std::cerr << dispPrefix << "FindItem: " << fmt_code(code) << " not found" << std::endl;
    
    return it_itm;
}

it_ord_t<OrderKw> Kiwoom::FindOrder(it_itm_t<ItemKw> it_itm, INT p, CSTR &ordno, bool nowarn)
{
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    it_ord_t<OrderKw> it_ord;
    if (it_itm != std::end(orderbook.items))
    {
        auto &ord = it_itm->second->ord;
        it_ord = std::end(ord);
        auto first_last = (p > 0 ? ord.equal_range(p)                      : // when p is known
                                   std::make_pair(std::begin(ord), it_ord)); // for cancels (p = 0)
        for (auto iO = first_last.first; iO != first_last.second; iO++)
        {
            if (iO->second.ordno == ordno)
            {
                it_ord = iO;
                break;
            }
        }
        if (it_ord == std::end(ord) && nowarn == false)
            std::cerr << dispPrefix << "FindOrder: " << fmt_code(it_itm->first) << " " << fmt_ordno(ordno) << " not found" << std::endl;
    }
    else
    {
        std::cerr << dispPrefix << "FindOrder: Called with an invalid iterator for orderbook.items" << std::endl;
        verify(false); // this may only happen via coding mistake and deserves a crash 
    }
    return it_ord;
}

}