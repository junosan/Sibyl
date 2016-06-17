/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_SERVER_ORDERBOOK_H_
#define SIBYL_SERVER_ORDERBOOK_H_

#include <vector>
#include <cctype>
#include <cinttypes>
#include <mutex>
#include <iostream>

#include "../Catalog.h"
#include "../util/DispPrefix.h"
#include "OrderBook_data.h"
#include "../ostream_format.h"

#include "../util/toggle_win32_min_max.h"

namespace sibyl
{

template <class TOrder, class TItem>
class OrderBook : public Catalog<TItem> //  /**/ mutex'd  
{
public:
    void SetVerbose(bool verbose_) { verbose = verbose_; }

    std::recursive_mutex items_mutex; // use when modifying items from an external class

/**/CSTR& BuildMsgOut      (bool addMyOrd);
/**/void  RemoveEmptyOrders(); // called every new tick
    const // correct/split a single UnnamedReq based on most up-to-date state
/**/std::vector<NamedReq<TOrder, TItem>>& AllotReq(UnnamedReq<TItem> req);
    
    // the following handles bal/cnt/ord/statistics (but doesn't remove orders even if q == 0)
/**/it_ord_t<TOrder> ApplyInsert(it_itm_t<TItem> iItems, TOrder o );
/**/void             ApplyTrade (it_itm_t<TItem> iItems, it_ord_t<TOrder> iOrd, PQ  pq); // Simulation: insert->trade for instant orders
/**/void             ApplyCancel(it_itm_t<TItem> iItems, it_ord_t<TOrder> iOrd, INT q );
    
    OrderBook() : verbose(false) {}
private:
    bool verbose;

    std::vector<NamedReq<TOrder, TItem>> nreq;
    STR msg;
};

template <class TOrder, class TItem>
CSTR& OrderBook<TOrder, TItem>::BuildMsgOut(bool addMyOrd)
{
    static char buf[1 << 12]; // sprintf line buf
    
    std::lock_guard<std::recursive_mutex> lock(items_mutex);
    
    msg.clear();
    
    // /*\n
    // b time bal sum.buy sum.sell sum.feetax
    sprintf(buf, "/*\nb %d %" PRId64 " %" PRId64 " %" PRId64 " %" PRId64 "\n", this->time.load(), this->bal, this->sum.buy, this->sum.sell, this->sum.feetax);
    msg.append(buf);
    
    // s sum.tck_orig.{bal q ord}[0..<(idx::szTb + 2)] (interleaved ordering)
    msg.append("s");
    for (const auto &s : this->sum.tck_orig) {
        sprintf(buf, " %" PRId64 " %" PRId64 " %" PRId64, s.bal, s.q, s.evt);
        msg.append(buf);
    }
    msg.append("\n");
    
    // k kospi200
    bool existELW = false;
    for (const auto &code_pItem : this->items)
        if (code_pItem.second->Type() == SecType::ELW) { existELW = true; break; }
    if (existELW == true)
    {
        if (ELW<TItem>::kospi200 <= 0.0f)
            std::cerr << dispPrefix << "OrderBook::BuildMsgOut: Nonpositive KOSPI200 index " << ELW<TItem>::kospi200 << std::endl; 
        sprintf(buf, "k %.5e\n", ELW<TItem>::kospi200);
        msg.append(buf);
    }
    
    // Individual items (d, (e), (n), o)
    for (const auto &code_pItem : this->items)
    {
        const auto &i = *code_pItem.second;
        
        std::array<PQ, idx::szTb> tbm = i.tbr; // tb_merged
        if (true == addMyOrd)
        {
            INT ps0 = i.Tck2P(-1, OrdType::sell); // unique in tb.p
            INT pb0 = i.Tck2P(-1, OrdType::buy ); // unique in tb.p
            for (auto iT = std::begin(tbm); iT != std::end(tbm); iT++)
            {
                std::ptrdiff_t idx = iT - std::begin(tbm);
                const auto &first_last = i.ord.equal_range(iT->p);
                for (auto iO = first_last.first; iO != first_last.second; iO++)
                {
                    const auto &o = iO->second;
                    if ( (idx <= idx::ps1 && o.type == OrdType::sell) ||
                         (idx >= idx::pb1 && o.type == OrdType::buy ) )
                        iT->q += o.q;
                }
                if (iT->p == ps0) iT->q = std::max(iT->q - i.depS0, 0);
                if (iT->p == pb0) iT->q = std::max(iT->q - i.depB0, 0);
            }
        }
        
        // d code pr qr tbr.p[0..<idx::szTb] tbr.q[0..<idx::szTb]
        sprintf(buf, "d %s %.5e %" PRId64 " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
                            code_pItem.first.c_str(), i.pr, i.qr,
                            tbm[ 0].p, tbm[ 1].p, tbm[ 2].p, tbm[ 3].p, tbm[ 4].p,
                            tbm[ 5].p, tbm[ 6].p, tbm[ 7].p, tbm[ 8].p, tbm[ 9].p, 
                            tbm[10].p, tbm[11].p, tbm[12].p, tbm[13].p, tbm[14].p,
                            tbm[15].p, tbm[16].p, tbm[17].p, tbm[18].p, tbm[19].p,
                            tbm[ 0].q, tbm[ 1].q, tbm[ 2].q, tbm[ 3].q, tbm[ 4].q,
                            tbm[ 5].q, tbm[ 6].q, tbm[ 7].q, tbm[ 8].q, tbm[ 9].q, 
                            tbm[10].q, tbm[11].q, tbm[12].q, tbm[13].q, tbm[14].q,
                            tbm[15].q, tbm[16].q, tbm[17].q, tbm[18].q, tbm[19].q );
        msg.append(buf);
        
        if (i.Type() == SecType::ELW)
        {
            auto &i = *static_cast<ELW<TItem>*>(code_pItem.second.get()); // reference as ELW<TItem>
            int CP = (i.CallPut() == OptType::call) - (i.CallPut() == OptType::put); 
            sprintf(buf, "e %s %+d %d %.5e %.5e %.5e %.5e %.5e %.5e %.5e %.5e\n",
                                code_pItem.first.c_str(), CP, i.Expiry(),
                                i.thr[0], i.thr[1], i.thr[2], i.thr[3], i.thr[4], i.thr[5], i.thr[6], i.thr[7]);
            msg.append(buf);
        }
        
        if (i.Type() == SecType::ETF)
        {
            auto &i = *static_cast<ETF<TItem>*>(code_pItem.second.get()); // reference as ETF<TItem>
            sprintf(buf, "n %s %.5e\n", code_pItem.first.c_str(), i.devNAV);
            msg.append(buf);
        }
        
        sprintf(buf, "o %s %d", code_pItem.first.c_str(), i.cnt);
        msg.append(buf);
        
        // Merge orders of the same price and list (+ for buy, - for sell)
        std::vector<PQ> ordm; // ord_merged
        for (auto iO = std::begin(i.ord); iO != std::end(i.ord);)
        {
            const auto &first_last = i.ord.equal_range(iO->first);
            auto MergeAndAdd = [&](OrdType type) {
                PQ pq;
                pq.p = iO->first;
                for (auto iP = first_last.first; iP != first_last.second; iP++) {
                    if (iP->second.type == type) pq.q += iP->second.q;
                }
                if (type == OrdType::sell) pq.q = -pq.q;
                if (pq.q != 0) ordm.push_back(pq);
            };
            MergeAndAdd(OrdType::buy );
            MergeAndAdd(OrdType::sell);
            iO = first_last.second;
        }

        for (const auto &pq : ordm)        
        {
            sprintf(buf, " %d %+d", pq.p, pq.q);
            msg.append(buf);
        }
        msg.append("\n");
    }
    msg.append("*/\n");
    
    return msg;
}

template <class TOrder, class TItem>
void OrderBook<TOrder, TItem>::RemoveEmptyOrders()
{
    std::lock_guard<std::recursive_mutex> lock(items_mutex);
    
    for (const auto &code_pItem : this->items)
    {
        auto &i = *code_pItem.second;
        for (auto iO = std::begin(i.ord); iO != std::end(i.ord);)
        {
            if (iO->second.q < 0)
                std::cerr << dispPrefix << "OrderBook::RemoveEmptyOrders: o.q " << fmt_quant(iO->second.q) << " < 0 found" << std::endl;  
            if (iO->second.q == 0) iO = i.ord.erase(iO);
            else                   iO++;
        }
    }
}

template <class TOrder, class TItem>
const std::vector<NamedReq<TOrder, TItem>>& OrderBook<TOrder, TItem>::AllotReq(UnnamedReq<TItem> req)
{
    std::lock_guard<std::recursive_mutex> lock(items_mutex);
    
    if (verbose == true)
    {
        std::cout << dispPrefix << "Allot:  " << req.type;
        if (req.type != ReqType::ca && req.type != ReqType::sa) std::cout << " " << fmt_code(req.iItems->first) << " " << fmt_price(req.p) << " " << fmt_quant(req.q);
        if (req.type == ReqType::mb || req.type == ReqType::ms) std::cout << " " << fmt_price(req.mp);
        std::cout << std::endl;
    }
    
    nreq.clear();
    
    // validation
    if (req.type != ReqType::ca && req.type != ReqType::sa)
    {
        auto &i = *(req.iItems->second);
        bool skip = false;
        
        // these should never fault unless requested by hand
        if ( (i.ValidP(req.p) == false)                     ||
             ((req.mp != 0) && (i.ValidP(req.mp) == false)) ||
             (req.p == req.mp)                              ||
             ((i.Type() == SecType::ELW) && (req.q % 10 != 0)) )
        {
            std::cerr << dispPrefix
                      << "OrderBook::AllotReq: invalid req " << req.type << " " << fmt_code(req.iItems->first.c_str()) << " "
                      << fmt_price(req.p) << " " << fmt_quant(req.q) << " " << fmt_price(req.mp) << std::endl;
            skip = true;
        }
        
        // this may happen naturally due rapid table shift
        if ( (((req.type == ReqType::b ) || (req.type == ReqType::s )) && 
              std::none_of(std::begin(i.tbr), std::end(i.tbr), [&req](const PQ & tb) { return tb.p == req.p;  })) ||
             (((req.type == ReqType::mb) || (req.type == ReqType::ms)) && 
              std::none_of(std::begin(i.tbr), std::end(i.tbr), [&req](const PQ & tb) { return tb.p == req.mp; })) )
        {
            skip = true;
        }
        
        if (skip == false)
        {
            if (req.type == ReqType::b)
            {
                req.p = std::min(req.p, i.Pb0());
                INT qmax = i.MaxBuyQ(this->bal, req.p);
                req.q = (req.q != 0 ? std::min(req.q, qmax) : qmax);
                if (req.q > 0)
                {
                    NamedReq<TOrder, TItem> temp;
                    temp.type   = ReqType::b;
                    temp.iItems = req.iItems;
                    temp.p      = req.p;
                    temp.q      = req.q;
                    nreq.push_back(temp);
                    if (verbose == true)
                        std::cout << dispPrefix
                                  << "                 -> " << fmt_price(req.p) << " " << fmt_quant(req.q) << std::endl; 
                }
            } else
            
            if (req.type == ReqType::s)
            {
                req.p = std::max(req.p, i.Ps0());
                req.q = (req.q != 0 ? std::min(req.q, i.cnt) : i.cnt);
                if (req.q > 0)
                {
                    NamedReq<TOrder, TItem> temp;
                    temp.type = ReqType::s;
                    temp.iItems = req.iItems;
                    temp.p    = req.p;
                    temp.q    = req.q;
                    nreq.push_back(temp);
                    if (verbose == true)
                        std::cout << dispPrefix
                                  << "                 -> " << fmt_price(req.p) << " " << fmt_quant(req.q) << std::endl;
                }
            } else
                
            if ((req.type == ReqType::cb) || (req.type == ReqType::cs) || (req.type == ReqType::mb) || (req.type == ReqType::ms))
            {
                INT  qleft   = req.q;
                
                if (req.type == ReqType::mb) req.mp = std::min(req.mp, i.Pb0());
                if (req.type == ReqType::ms) req.mp = std::max(req.mp, i.Ps0());
                if (req.p == req.mp) skip = true;
                
                bool limited = true;
                if ((req.type == ReqType::mb) && (req.mp > req.p))
                {
                    INT qmax = i.MaxBuyQ(this->bal, req.mp - req.p);
                    qleft = (req.q != 0 ? std::min(req.q, qmax) : qmax);
                    if (qleft <= 0) skip = true;
                }
                else if (req.q == 0) limited = false;
                    
                if (skip == false)
                {
                    auto first_last = i.ord.equal_range(req.p);
                    typename std::multimap<INT, TOrder>::reverse_iterator rBeg(first_last.second);
                    typename std::multimap<INT, TOrder>::reverse_iterator rEnd(first_last.first );
                    for (auto riO = rBeg; riO != rEnd; riO++)
                    {
                        const auto &o = riO->second;
                        INT deltaq = 0;
                        if (limited == true) qleft -= (deltaq = std::min(o.q, qleft));
                        else                           deltaq = o.q;
                        
                        if (deltaq > 0)
                        {
                            auto iO = riO.base();
                            iO--; // so that first element is *(second - 1) and last element is *first
                            
                            NamedReq<TOrder, TItem> temp;
                            temp.type   = req.type;
                            temp.iItems = req.iItems;
                            if ((req.type == ReqType::mb) || (req.type == ReqType::ms)) temp.p = req.mp;
                            temp.q      = deltaq;
                            temp.iOrd   = iO;
                            nreq.push_back(temp);
                            if (verbose == true)
                            {
                                std::cout << dispPrefix
                                          << "                 -> " << fmt_quant(deltaq);
                                if ((req.type == ReqType::mb) || (req.type == ReqType::ms))
                                    std::cout << " " << fmt_price(req.mp);
                                std::cout << std::endl;
                            }
                        }
                        
                        if ((limited == true) && (qleft <= 0)) break;
                    }
                }
            }
        }
    } else

    if (req.type == ReqType::ca)
    {
        NamedReq<TOrder, TItem> temp;
        for (auto iItems = std::begin(this->items), end = std::end(this->items); iItems != end; ++iItems)
        {
            auto &i = *iItems->second;
            for (auto iO = std::begin(i.ord); iO != std::end(i.ord); iO++)
            {
                const auto &o = iO->second;
                if (o.q > 0)
                {
                    if (o.type == OrdType::buy ) temp.type = ReqType::cb;
                    if (o.type == OrdType::sell) temp.type = ReqType::cs;
                    temp.iItems = iItems;
                    temp.q      = o.q;
                    temp.iOrd   = iO;
                    nreq.push_back(temp);
                    if (verbose == true)
                    {
                        std::cout << dispPrefix
                                  << "                 -> " << temp.type << " " << fmt_code(iItems->first) << " " << fmt_price(o.p) << " " << fmt_quant(o.q) << std::endl;
                    }
                }
            }
        }
    } else
    
    if (req.type == ReqType::sa)
    {
        NamedReq<TOrder, TItem> temp;
        for (auto iItems = std::begin(this->items), end = std::end(this->items); iItems != end; ++iItems)
        {
            auto &i = *iItems->second;
            if (i.cnt > 0)
            {
                temp.type = ReqType::s;
                temp.iItems = iItems;
                temp.p      = i.Ps0();
                temp.q      = i.cnt;
                nreq.push_back(temp);
                if (verbose == true)
                    std::cout << dispPrefix
                              << "                 ->  s " << fmt_code(iItems->first) << " " << fmt_price(temp.p) << " " << fmt_quant(temp.q) << std::endl;
            }
        }
    }
    
    return nreq;
}

template <class TOrder, class TItem>
it_ord_t<TOrder> OrderBook<TOrder, TItem>::ApplyInsert(it_itm_t<TItem> iItems, TOrder o)
{
    std::lock_guard<std::recursive_mutex> lock(items_mutex);
    
    if (o.q <= 0)
    {
        std::cerr << dispPrefix << "OrderBook::ApplyInsert: " << fmt_code(iItems->first) << " o.q " << fmt_quant(o.q) << " <= 0 found" << std::endl;
        o.q = 0;
    }
       
    auto &i = *iItems->second;
    if (verbose == true) std::cout << dispPrefix << "<Insert> " << o.type << " " << fmt_code(iItems->first) << " " << fmt_price(o.p) << " " << fmt_quant(o.q) << std::endl;
    
    if (o.type == OrdType::buy)
    {
        if (verbose == true) std::cout << dispPrefix << "    [bal]: " << fmt_bal(this->bal) << " [-] " << fmt_price(o.p) << " * " <<  fmt_quant(o.q) << " * (1 + f_b) = ";
        INT64 raw = (INT64)o.p * o.q;
        this->bal -= raw + i.BFee(raw);
        if (verbose == true) std::cout << fmt_bal(this->bal) << std::endl;
        if (this->bal < 0) std::cerr << dispPrefix << "OrderBook::ApplyInsert: Nagative bal reached" << std::endl;
    } else
    if (o.type == OrdType::sell)
    {
        if (verbose == true) std::cout << dispPrefix << "    [cnt]: " << fmt_code(iItems->first) << " " << fmt_quant(i.cnt) << " [-] " << fmt_quant(o.q) << " = "; 
        i.cnt -= o.q;
        if (verbose == true) std::cout << fmt_quant(i.cnt) << std::endl;
        if (i.cnt < 0) std::cerr << dispPrefix << "OrderBook::ApplyInsert: Nagative cnt reached" << std::endl;
    }
    
    o.tck_orig = i.P2Tck(o.p, o.type);
    auto iOrd = i.ord.insert(std::make_pair(o.p, o));
    
    return iOrd;
}

template <class TOrder, class TItem>
void OrderBook<TOrder, TItem>::ApplyTrade (it_itm_t<TItem> iItems, it_ord_t<TOrder> iOrd, PQ pq)
{
    std::lock_guard<std::recursive_mutex> lock(items_mutex);
    
    if (pq.q > 0)
    {
        auto &i = *iItems->second;
        auto &o = iOrd->second;
        if (verbose == true) std::cout << dispPrefix << "<Trade>  " << o.type << " " << fmt_code(iItems->first) << " " << fmt_price(o.p) << " " << fmt_quant(o.q) << " [-] " << fmt_price(pq.p) << " " << fmt_quant(pq.q) << " = " << fmt_quant(o.q - pq.q) << std::endl; 
        if (o.type == OrdType::buy)
        {
            if (o.p < pq.p)
            {
                std::cerr << dispPrefix << "OrderBook::ApplyTrade: " << fmt_code(iItems->first) << " o.p " << fmt_price(o.p) << " < delta.p " << fmt_price(pq.p) << " for buy order found" << std::endl;
            }
            
            if (o.p > pq.p) // if traded price was lower than requested
            {
                if (verbose == true) std::cout << dispPrefix << "    [bal]: " << fmt_bal(this->bal) << " [+] " << fmt_price(o.p - pq.p) << " * " << fmt_quant(pq.q) << " * (1 + f_b) = ";
                INT64 raw = (INT64)(o.p - pq.p) * pq.q;
                this->bal += raw + i.BFee(raw);
                if (verbose == true) std::cout << fmt_bal(this->bal) << std::endl;
            }
            
            if (verbose == true) std::cout << dispPrefix << "    [cnt]: " << fmt_code(iItems->first) << " " << fmt_quant(i.cnt) << " [+] " << fmt_quant(pq.q) << " = ";
            i.cnt += pq.q;
            if (verbose == true) std::cout << fmt_quant(i.cnt) << std::endl;
            
            INT64 raw = (INT64)pq.p * pq.q;
            this->sum.buy    += raw;
            this->sum.feetax += i.BFee(raw);
            
            if ((o.tck_orig >= -1) && (o.tck_orig < idx::tckN)) {
                int idx = (o.tck_orig != -1 ? idx::pb1 + o.tck_orig : this->idxTckOrigB0);
                this->sum.tck_orig[(std::size_t)idx].bal += raw + i.BFee(raw);
                this->sum.tck_orig[(std::size_t)idx].q   += pq.q;
                this->sum.tck_orig[(std::size_t)idx].evt += 1;
            }
        } else
        if (o.type == OrdType::sell)
        {
            if (o.p > pq.p)
            {
                std::cerr << dispPrefix << "OrderBook::ApplyTrade: " << fmt_code(iItems->first) << " o.p " << fmt_price(o.p) << " > delta.p " << fmt_price(pq.p) << " for sell order found" << std::endl;
            }
            
            if (verbose == true) std::cout << dispPrefix << "    [bal]: " << fmt_bal(this->bal) << " [+] " << fmt_price(pq.p) << " * " << fmt_quant(pq.q) << " * (1 - f_s) = ";
            INT64 raw = (INT64)pq.p * pq.q;
            this->bal += raw - i.SFee(raw);
            if (verbose == true) std::cout << fmt_bal(this->bal) << std::endl;
            
            this->sum.sell   += raw;
            this->sum.feetax += i.SFee(raw);
            
            if ((o.tck_orig >= -1) && (o.tck_orig < idx::tckN)) {
                int idx = (o.tck_orig != -1 ? idx::ps1 - o.tck_orig : this->idxTckOrigS0);
                this->sum.tck_orig[(std::size_t)idx].bal += raw - i.SFee(raw);
                this->sum.tck_orig[(std::size_t)idx].q   += pq.q;
                this->sum.tck_orig[(std::size_t)idx].evt += 1;
            }
        }
        o.q -= pq.q;
        if (o.q < 0) std::cerr << dispPrefix << "OrderBook::ApplyTrade: Negative order q reached" << std::endl;
    }
}

template <class TOrder, class TItem>
void OrderBook<TOrder, TItem>::ApplyCancel(it_itm_t<TItem> iItems, it_ord_t<TOrder> iOrd, INT q)
{
    std::lock_guard<std::recursive_mutex> lock(items_mutex);
    
    if (q > 0)
    {
        auto &i = *iItems->second;
        auto &o = iOrd->second;
        if (verbose == true) std::cout << dispPrefix << "<Cancel> " << o.type << " " << fmt_code(iItems->first) << " " << fmt_price(o.p) << " " << fmt_quant(o.q) << " [-] " << fmt_quant(q) << " = " << fmt_quant(o.q - q) << std::endl;
        
        if (o.type == OrdType::buy)
        {
            if (verbose == true) std::cout << dispPrefix << "    [bal]: " << fmt_bal(this->bal) << " [+] " << fmt_price(o.p) << " * " << fmt_quant(q) << " * (1 + f_b) = ";
            INT64 raw = (INT64)o.p * q;
            this->bal += raw + i.BFee(raw);
            if (verbose == true) std::cout << fmt_bal(this->bal) << std::endl;
        } else
        if (o.type == OrdType::sell)
        {
            if (verbose == true) std::cout << dispPrefix << "    [cnt]: " << fmt_code(iItems->first) << " " << fmt_quant(i.cnt) << " [+] " << fmt_quant(q) << " = ";
            i.cnt += q;
            if (verbose == true) std::cout << fmt_quant(i.cnt) << std::endl;
        }
        o.q -= q;
        if (o.q < 0) std::cerr << dispPrefix << "OrderBook::ApplyCancel: Negative order q reached" << std::endl;
    }
}

}

#include "../util/toggle_win32_min_max.h"

#endif /* SIBYL_SERVER_ORDERBOOK_H_ */