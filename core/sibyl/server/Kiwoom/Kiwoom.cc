
#include <thread>
#include <fstream>

#include "Kiwoom.h"

namespace sibyl
{

// static
Clock Kiwoom::clock;
void  (*Kiwoom::SetInputValue)(InputKey, CSTR&)                  = nullptr;
long  (*Kiwoom::CommRqData)   (CSTR&, CSTR&, long)               = nullptr;
CSTR& (*Kiwoom::GetCommData)  (CSTR&, CSTR&, CommDataKey)        = nullptr;
long  (*Kiwoom::SendOrder)    (CSTR&, ReqType, CSTR&, PQ, CSTR&) = nullptr;

void Kiwoom::SetStateFile(CSTR &filename)
{
    {
        std::ofstream ofs(filename);
        verify(ofs.is_open() == true);
    }
    stateFileName = filename;
}

int Kiwoom::AdvanceTick()
{
    int timeTarget = GetOrderBookTime() + kTimeTickSec; // s from 09:00:00
    int t_target = (timeTarget - timeOffset) * 1000; // ms from 00:00:00
    int t_wait   = t_target - (clock.Now() + t_data_minus_local); // synchronize to data time
    
    if (t_wait <= -kTimeTickSec * 1000)
        skipTick = true; // skip time tick if more than one tick behind
    else
    {
        if (t_wait > 0) std::this_thread::sleep_for(std::chrono::milliseconds(t_wait));
        skipTick = false;
        ResetInterrupt();
    }
    
    SetOrderBookTime(timeTarget);    
    
    return (GetOrderBookTime() < timeBounds.end ? 0 : -1);
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
    
    const int  nItemPerLine = 4;
    const char itemSpacer[] = " ";
    static char buf[1 << 8];
    
    ofs << "[t=" << std::setw(5) << GetOrderBookTime() << "]\n";
    ofs << "bal " << std::setw(12) << orderbook.bal << "\n";
    auto evl = orderbook.Evaluate();
    ofs << "evl " << std::setw(12) << evl.evalTot;
    sprintf(buf, " (r%+.2f%%) (s%+.2f%%)\n", orderbook.GetProfitRate(true), orderbook.GetProfitRate(false));
    
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
                    if (tck == szTck) tck = 98;     // display as 99 if not found
                    sprintf(buf, "[%s%2d] {%s} %8d (%6d)", (type == kOrdBuy ? "b" : "s"), tck + 1, code_pItem.first.c_str(), o.p, o.q);
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
    
    ListOrder(kOrdBuy) ;
    ListOrder(kOrdSell);
    ofs << std::endl;
}

void Kiwoom::ApplyRealtimeTr(CSTR &code, INT p, INT q, INT trPs1, INT trPb1)
{
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
        i.trPs1 = trPs1;
        i.trPb1 = trPb1;
    }
}

void Kiwoom::ApplyRealtimeTb(CSTR &code, CSTR &time, const std::array<PQ, szTb> &tb)
{
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    
    static STR lastTime;
    if (lastTime != time) // update at new second (i.e., closest to HHMMSS.000)
    {
        int t_data = Clock::HHMMSS_to_ms(time);
        t_data_minus_local = t_data - clock.Now();
        if (t_data / 1000 + timeOffset >= GetOrderBookTime() + kTimeTickSec)
            InterruptExec();
        lastTime = time;
    }
    
    auto it_itm = FindItem(code);
    if (it_itm != std::end(orderbook.items))
        it_itm->second->tb = tb;
}

void Kiwoom::ApplyRealtimeNAV(CSTR &code, INT p, FLOAT nav)
{
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    auto it_itm = FindItem(code);
    if (it_itm != std::end(orderbook.items))
    {
        auto &i = *it_itm->second;
        if (i.Type() == kSecETF) 
            dynamic_cast<ETF<ItemKw>&>(i).devNAV = (FLOAT) (((double) nav / p - 1.0) * 100.0);
        else
            std::cerr << dispPrefix << "ApplyRealtimeNAV: {" << code << "} is not ETF" << std::endl;
    }
}

void Kiwoom::ApplyReqEvent(CSTR &code, ReqStat reqStat, ReqType reqType,
                           CSTR &ordno, INT ordp, INT ordq,
                           CSTR &ordno_o, INT delta_p, INT delta_q)
{
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);

    // pre-filter non-existent item
    auto it_itm = FindItem(code);
    if (it_itm == std::end(orderbook.items)) return;

    auto OrderExists = [&](const it_ord_t<OrderKw> &it_ord) {
        return it_ord != std::end(it_itm->second->ord);
    };

    if (reqStat == ReqStat::received)
    {
        if (reqType != kReq_cb && reqType != kReq_cs)
        {
            if (ordq > 0) // avoid re-receiving emptied order after c | m
            {
                auto it_ord = FindOrder(it_itm, ordp, ordno);
                if (OrderExists(it_ord) == false)
                { 
                    if (reqType == kReq_mb || reqType == kReq_ms) // trim original order
                    {
                        auto it_ord_o = FindOrder(it_itm, 0, ordno_o);
                        if (OrderExists(it_ord_o) == true)
                            orderbook.ApplyCancel(it_itm, it_ord_o, ordq);
                        else
                            std::cerr << dispPrefix << "ApplyReqEvent: mb | ms event received on a non-existent order" << std::endl;
                    }
                    
                    OrderKw o;
                    o.type  = (reqType == kReq_b || reqType == kReq_mb ? kOrdBuy : kOrdSell);
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
                std::cerr << dispPrefix << "ApplyReqEvent: cb | cs event received on a non-existent order" << std::endl;
        }
    }
    else if (reqStat == ReqStat::confirmed)
    {
        return;
    }
    else if (reqStat == ReqStat::traded)
    {
        auto it_ord = FindOrder(it_itm, ordp, ordno);
        if (OrderExists(it_ord) == true)
            orderbook.ApplyTrade(it_itm, it_ord, PQ(delta_p, delta_q));
        else
            std::cerr << dispPrefix << "ApplyReqEvent: Trade event received on a non-existent order" << std::endl;
    }
}

void Kiwoom::ApplyCntEvent(CSTR &code, INT cnt)
{
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    auto it_itm = FindItem(code);
    if (it_itm != std::end(orderbook.items))
    {
        auto &i = *it_itm->second;
        if (i.cnt != cnt)
        {
            // Note: this may trigger naturally if a req is sent by me & received by server
            //       in between a pair of ReqEvent & CntEvent
            std::cerr << dispPrefix << "ApplyCntEvent: {" << code << "} item.cnt (" << i.cnt << ") != event.cnt (" << cnt << ")" << std::endl;
            // i.cnt = cnt;
        }
    }
}

int Kiwoom::ExecuteNamedReq(NamedReq<OrderKw, ItemKw> req)
{
    // do nothing (for now)
    
    // correct q
    
    // send req
    
    return 0;
}

it_itm_t<ItemKw> Kiwoom::FindItem(CSTR &code)
{
    std::lock_guard<std::recursive_mutex> lock(orderbook.items_mutex);
    auto it_itm = orderbook.items.find(code); 
    if (it_itm == std::end(orderbook.items))
        std::cerr << dispPrefix << "FindItem: {" << code << "} not found" << std::endl;
    return it_itm;
}

it_ord_t<OrderKw> Kiwoom::FindOrder(it_itm_t<ItemKw> it_itm, INT p, CSTR &ordno)
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
        if (it_ord == std::end(ord))
            std::cerr << dispPrefix << "FindOrder: {" << it_itm->first << "} #" << ordno << " not found at " << p << std::endl;
    }
    else
    {
        std::cerr << dispPrefix << "FindOrder: Called with an invalid iterator for orderbook.items" << std::endl;
        verify(false); // this may only happen via coding mistake and deserves a crash 
    }
    return it_ord;
}

}