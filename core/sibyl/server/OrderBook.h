#ifndef SIBYL_SERVER_ORDERBOOK_H_
#define SIBYL_SERVER_ORDERBOOK_H_

#include <map>
#include <vector>
#include <cctype>
#include <cinttypes>
#include <mutex>
#include <iostream>

#include "../Security.h"
#include "../Security_KOSPI.h"
#include "../Security_ELW.h"
#include "../Catalog.h"

namespace sibyl
{

// shorthand for Security.ord's iterator
template <class TOrder>
using it_ord_t = typename std::multimap<INT, TOrder>::iterator; 

// shorthand for Catalog.items's iterator
template <class TItem>
using it_itm_t = typename std::map<STR, std::unique_ptr<TItem>>::iterator;

class Order : public PQ
{
public:
    OrdType type;
    int     tck_orig; // to be automatically filled by ApplyInsert function
    Order()             :           type(kOrdNull), tck_orig(szTck) {}
    Order(INT p, INT q) : PQ(p, q), type(kOrdNull), tck_orig(szTck) {}
};

template <class TOrder>
class Item : public Security<TOrder>
{
public:
    virtual ~Item() {}
};

template <class TItem>
class UnnamedReq
{
public:
    ReqType         type;
    it_itm_t<TItem> iItems;
    INT             p;
    INT             q;
    INT             mp; // only for mb, ms
    UnnamedReq() : type(kReqNull), p(0), q(0), mp(0) {}
};

template <class TOrder, class TItem>
class NamedReq
{
public:
    ReqType          type;
    it_itm_t<TItem>  iItems;
    it_ord_t<TOrder> iOrd; // only for cb, cs, mb, ms
    INT              p;    // only for b, s, mb, ms (p = mp for mb|ms)
    INT              q;
    NamedReq() : type(kReqNull), p(0), q(0) {}
};

template <class TOrder, class TItem>
class OrderBook : public Catalog<TItem> //  /**/ mutex'd  
{
public:
    void SetVerbose(bool verbose_) { verbose = verbose_; }

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
    
    std::recursive_mutex mutexData;

    std::vector<NamedReq<TOrder, TItem>> nreq;
    STR msg;
    char bufLine[1 << 12]; // sprintf line buf
};

template <class TOrder, class TItem>
CSTR& OrderBook<TOrder, TItem>::BuildMsgOut(bool addMyOrd)
{
    std::lock_guard<std::recursive_mutex> lock(mutexData);
    
    msg.clear();
    
    // /*\n
    // b time bal sum.buy sum.sell sum.feetax
    sprintf(bufLine, "/*\nb %d %" PRId64 " %" PRId64 " %" PRId64 " %" PRId64 "\n", this->time, this->bal, this->sum.buy, this->sum.sell, this->sum.feetax);
    msg.append(bufLine);
    
    // s sum.tck_orig.{bal q ord}[0..<(szTb + 2)] (interleaved ordering)
    msg.append("s");
    for (const auto &s : this->sum.tck_orig) {
        sprintf(bufLine, " %" PRId64 " %" PRId64 " %" PRId64, s.bal, s.q, s.evt);
        msg.append(bufLine);
    }
    msg.append("\n");
    
    // k kospi200
    bool existELW = false;
    for (const auto &code_pItem : this->items)
        if (code_pItem.second->Type() == kSecELW) { existELW = true; break; }
    if (existELW == true)
    {
        assert(ELW<TItem>::kospi200 > 0.0f);
        sprintf(bufLine, "k %.5e\n", ELW<TItem>::kospi200);
        msg.append(bufLine);
    }
    
    // Individual items (d, (e), o)
    for (const auto &code_pItem : this->items)
    {
        const auto &i = *code_pItem.second;
        
        std::array<PQ, szTb> tbm = i.tbr; // tb_merged
        if (true == addMyOrd)
        {
            for (auto iT = std::begin(tbm); iT != std::end(tbm); iT++)
            {
                std::ptrdiff_t idx = iT - std::begin(tbm);
                const auto &first_last = i.ord.equal_range(iT->p);
                for (auto iO = first_last.first; iO != first_last.second; iO++)
                {
                    const auto &o = iO->second;
                    if ( ((idx <= idxPs1) && (o.type == kOrdSell)) ||
                         ((idx >= idxPb1) && (o.type == kOrdBuy )) )
                        iT->q += o.q;
                }
            }
        }
        
        // d code pr qr tbr.p[0..<szTb] tbr.q[0..<szTb]
        sprintf(bufLine, "d %s %.5e %" PRId64 " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
                            code_pItem.first.c_str(), i.pr, i.qr,
                            tbm[ 0].p, tbm[ 1].p, tbm[ 2].p, tbm[ 3].p, tbm[ 4].p,
                            tbm[ 5].p, tbm[ 6].p, tbm[ 7].p, tbm[ 8].p, tbm[ 9].p, 
                            tbm[10].p, tbm[11].p, tbm[12].p, tbm[13].p, tbm[14].p,
                            tbm[15].p, tbm[16].p, tbm[17].p, tbm[18].p, tbm[19].p,
                            tbm[ 0].q, tbm[ 1].q, tbm[ 2].q, tbm[ 3].q, tbm[ 4].q,
                            tbm[ 5].q, tbm[ 6].q, tbm[ 7].q, tbm[ 8].q, tbm[ 9].q, 
                            tbm[10].q, tbm[11].q, tbm[12].q, tbm[13].q, tbm[14].q,
                            tbm[15].q, tbm[16].q, tbm[17].q, tbm[18].q, tbm[19].q );
        msg.append(bufLine);
        
        if (i.Type() == kSecELW)
        {
            auto &i = *dynamic_cast<ELW<TItem>*>(code_pItem.second.get()); // reference as ELW<TItem>
            int CP = (i.CallPut() == kOptCall) - (i.CallPut() == kOptPut); 
            sprintf(bufLine, "e %s %+d %d %.5e %.5e %.5e %.5e %.5e %.5e %.5e %.5e\n",
                                code_pItem.first.c_str(), CP, i.Expiry(),
                                i.thr[0], i.thr[1], i.thr[2], i.thr[3], i.thr[4], i.thr[5], i.thr[6], i.thr[7]);
            msg.append(bufLine);
        }
        
        sprintf(bufLine, "o %s %d", code_pItem.first.c_str(), i.cnt);
        msg.append(bufLine);
        
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
                if (type == kOrdSell) pq.q = -pq.q;
                if (pq.q != 0) ordm.push_back(pq);
            };
            MergeAndAdd(kOrdBuy );
            MergeAndAdd(kOrdSell);
            iO = first_last.second;
        }

        for (const auto &pq : ordm)        
        {
            sprintf(bufLine, " %d %+d", pq.p, pq.q);
            msg.append(bufLine);
        }
        msg.append("\n");
    }
    msg.append("*/\n");
    
    return msg;
}

template <class TOrder, class TItem>
void OrderBook<TOrder, TItem>::RemoveEmptyOrders()
{
    std::lock_guard<std::recursive_mutex> lock(mutexData);
    
    for (const auto &code_pItem : this->items)
    {
        auto &i = *code_pItem.second;
        for (auto iO = std::begin(i.ord); iO != std::end(i.ord);)
        {
            assert(iO->second.q >= 0); // there should be no negative order 
            if (iO->second.q == 0) iO = i.ord.erase(iO);
            else                   iO++;
        }
    }
}

template <class TOrder, class TItem>
const std::vector<NamedReq<TOrder, TItem>>& OrderBook<TOrder, TItem>::AllotReq(UnnamedReq<TItem> req)
{
    std::lock_guard<std::recursive_mutex> lock(mutexData);
    
    if (verbose == true)
    {
        std::cout << "Allot:   ";
        if      (req.type == kReq_b ) std::cout << "b";
        else if (req.type == kReq_s ) std::cout << "s";
        else if (req.type == kReq_cb) std::cout << "cb";
        else if (req.type == kReq_cs) std::cout << "cs";
        else if (req.type == kReq_mb) std::cout << "mb";
        else if (req.type == kReq_ms) std::cout << "ms";
        std::cout << " {" << req.iItems->first << "} " << req.p << " (" << req.q << ")";
        if (req.type == kReq_mb || req.type == kReq_ms) std::cout << " " << req.mp;
        std::cout << std::endl;
    }
    
    nreq.clear();
    
    // validation
    if ((req.type != kReq_ca) && (req.type != kReq_sa))
    {
        auto &i = *(req.iItems->second);
        bool skip = false;
        
        // these should never fault unless requested by hand
        if ( (i.ValidP(req.p) == false)                     ||
             ((req.mp != 0) && (i.ValidP(req.mp) == false)) ||
             (req.p == req.mp)                              ||
             ((i.Type() == kSecELW) && (req.q % 10 != 0)) )
        {
            std::cerr << "OrderBook.AllotReq: invalid req.p or req.q" << std::endl;
            skip = true;
        }
        
        // this may happen naturally due rapid table shift
        if ( (((req.type == kReq_b ) || (req.type == kReq_s )) && 
              std::none_of(std::begin(i.tbr), std::end(i.tbr), [&req](const PQ & tb) { return tb.p == req.p;  })) ||
             (((req.type == kReq_mb) || (req.type == kReq_ms)) && 
              std::none_of(std::begin(i.tbr), std::end(i.tbr), [&req](const PQ & tb) { return tb.p == req.mp; })) )
        {
            skip = true;
        }
        
        if (skip == false)
        {
            if (req.type == kReq_b)
            {
                req.p = std::min(req.p, i.Pb0());
                INT64 qmax64 = (INT64)std::floor(this->bal / (req.p * (1.0 + i.dBF())));
                INT   qmax   =   (INT)std::min(qmax64, (INT64)0x7FFFFFFF);; 
                if (i.Type() == kSecELW) qmax -= qmax % 10;
                req.q = (req.q != 0 ? std::min(req.q, qmax) : qmax);
                if (req.q > 0)
                {
                    NamedReq<TOrder, TItem> temp;
                    temp.type   = kReq_b;
                    temp.iItems = req.iItems;
                    temp.p      = req.p;
                    temp.q      = req.q;
                    nreq.push_back(temp);
                    if (verbose == true) std::cout << "                 -> " << req.p << " (" << req.q << ")" << std::endl; 
                }
            } else
            
            if (req.type == kReq_s)
            {
                req.p = std::max(req.p, i.Ps0());
                req.q = (req.q != 0 ? std::min(req.q, i.cnt) : i.cnt);
                if (req.q > 0)
                {
                    NamedReq<TOrder, TItem> temp;
                    temp.type = kReq_s;
                    temp.iItems = req.iItems;
                    temp.p    = req.p;
                    temp.q    = req.q;
                    nreq.push_back(temp);
                    if (verbose == true) std::cout << "                 -> " << req.p << " (" << req.q << ")" << std::endl;
                }
            } else
                
            if ((req.type == kReq_cb) || (req.type == kReq_cs) || (req.type == kReq_mb) || (req.type == kReq_ms))
            {
                INT  qleft   = req.q;
                
                if (req.type == kReq_mb) req.mp = std::min(req.mp, i.Pb0());
                if (req.type == kReq_ms) req.mp = std::max(req.mp, i.Ps0());
                if (req.p == req.mp) skip = true;
                
                bool limited = true;
                if ((req.type == kReq_mb) && (req.mp > req.p))
                {
                    INT64 qmax64 = (INT64)std::floor(this->bal / ((req.mp - req.p) * (1.0 + i.dBF())));
                    INT   qmax   =   (INT)std::min(qmax64, (INT64)0x7FFFFFFF);; 
                    if (i.Type() == kSecELW) qmax -= qmax % 10;
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
                            if ((req.type == kReq_mb) || (req.type == kReq_ms)) temp.p = req.mp;
                            temp.q      = deltaq;
                            temp.iOrd   = iO;
                            nreq.push_back(temp);
                            if (verbose == true)
                            {
                                std::cout << "                 -> (" << deltaq << ")";
                                if ((req.type == kReq_mb) || (req.type == kReq_ms))
                                    std::cout << " " << req.mp;
                                std::cout << std::endl;
                            }
                        }
                        
                        if ((limited == true) && (qleft <= 0)) break;
                    }
                }
            }
        }
    } else

    if (req.type == kReq_ca)
    {
        NamedReq<TOrder, TItem> temp;
        for (const auto &code_pItem : this->items)
        {
            auto &i = *code_pItem.second;
            for (auto iO = std::begin(i.ord); iO != std::end(i.ord); iO++)
            {
                const auto &o = iO->second;
                if (o.q > 0)
                {
                    if (o.type == kOrdBuy ) temp.type = kReq_cb;
                    if (o.type == kOrdSell) temp.type = kReq_cs;
                    temp.iItems = req.iItems;
                    temp.q      = o.q;
                    temp.iOrd   = iO;
                    nreq.push_back(temp);
                    if (verbose == true)
                    {
                        std::cout << "                 -> ";
                        if (o.type == kOrdBuy ) std::cout << "cb";
                        if (o.type == kOrdSell) std::cout << "cs";
                        std::cout << " {" << code_pItem.first << "} " << o.p << " (" << o.q << ")" << std::endl;
                    }
                }
            }
        }
    } else
    
    if (req.type == kReq_sa)
    {
        NamedReq<TOrder, TItem> temp;
        for (const auto &code_pItem : this->items)
        {
            const auto &i = *code_pItem.second;
            if (i.cnt > 0)
            {
                temp.type = kReq_s;
                temp.iItems = req.iItems;
                temp.p      = i.Ps0();
                temp.q      = i.cnt;
                nreq.push_back(temp);
                if (verbose == true) std::cout << "                 -> s {" << code_pItem.first << "} " << temp.p << " (" << temp.q << ")" << std::endl;
            }
        }
    }
    
    return nreq;
}

template <class TOrder, class TItem>
it_ord_t<TOrder> OrderBook<TOrder, TItem>::ApplyInsert(it_itm_t<TItem> iItems, TOrder o)
{
    std::lock_guard<std::recursive_mutex> lock(mutexData);
    
    assert(o.q > 0);   
    auto &i = *iItems->second;
    if (verbose == true) std::cout << "<Insert> " << (o.type == kOrdBuy ? "b" : "s") << " {" << iItems->first << "} " << o.p << " (" << o.q << ")" << std::endl;
    
    if (o.type == kOrdBuy)
    {
        if (verbose == true) std::cout << "    [bal]: " << this->bal << " [-] " << o.p << " * (" <<  o.q << ") * (1 + f_b) = ";
        INT64 raw = (INT64)o.p * o.q;
        this->bal -= raw + i.BFee(raw);
        if (verbose == true) std::cout << this->bal << std::endl;
        if (this->bal < 0) std::cerr << "OrderBook.ApplyInsert: Nagative bal reached" << std::endl;
    } else
    if (o.type == kOrdSell)
    {
        if (verbose == true) std::cout << "    [cnt]: {" << iItems->first << "} (" << i.cnt << ") [-] (" << o.q << ") = "; 
        i.cnt -= o.q;
        if (verbose == true) std::cout << i.cnt << std::endl;
        if (i.cnt < 0) std::cerr << "OrderBook.ApplyInsert: Nagative cnt reached" << std::endl;
    }
    
    o.tck_orig = i.P2Tck(o.p, o.type);
    auto iOrd = i.ord.insert(std::make_pair(o.p, o));
    
    return iOrd;
}

template <class TOrder, class TItem>
void OrderBook<TOrder, TItem>::ApplyTrade (it_itm_t<TItem> iItems, it_ord_t<TOrder> iOrd, PQ pq)
{
    std::lock_guard<std::recursive_mutex> lock(mutexData);
    
    if (pq.q > 0)
    {
        auto &i = *iItems->second;
        auto &o = iOrd->second;
        if (verbose == true) std::cout << "<Trade>  " << (o.type == kOrdBuy ? "b" : "s") << " {" << iItems->first << "} " << o.p << " (" << o.q << ") [-] " << pq.p << " (" << pq.q << ") = (" << o.q - pq.q << ")" << std::endl; 
        if (o.type == kOrdBuy)
        {
            assert(o.p >= pq.p);
            if (o.p > pq.p) // if traded price was lower than requested
            {
                if (verbose == true) std::cout << "    [bal]: " << this->bal << " [+] " << (o.p - pq.p) << " * (" << pq.q << ") * (1 + f_b) = ";
                INT64 raw = (INT64)(o.p - pq.p) * pq.q;
                this->bal += raw + i.BFee(raw);
                if (verbose == true) std::cout << this->bal << std::endl;
            }
            
            if (verbose == true) std::cout << "    [cnt]: {" << iItems->first << "} (" << i.cnt << ") [+] (" << pq.q << ") = ";
            i.cnt += pq.q;
            if (verbose == true) std::cout << i.cnt << std::endl;
            
            INT64 raw = (INT64)pq.p * pq.q;
            this->sum.buy    += raw;
            this->sum.feetax += i.BFee(raw);
            
            if ((o.tck_orig >= -1) && (o.tck_orig < szTck)) {
                int idx = (o.tck_orig != -1 ? idxPb1 + o.tck_orig : idxTckOrigB0);
                this->sum.tck_orig[(std::size_t)idx].bal += raw + i.BFee(raw);
                this->sum.tck_orig[(std::size_t)idx].q   += pq.q;
                this->sum.tck_orig[(std::size_t)idx].evt += 1;
            }
        } else
        if (o.type == kOrdSell)
        {
            assert(o.p <= pq.p);
            if (verbose == true) std::cout << "    [bal]: " << this->bal << " [+] " << pq.p << " * (" << pq.q << ") * (1 - f_s) = ";
            INT64 raw = (INT64)pq.p * pq.q;
            this->bal += raw - i.SFee(raw);
            if (verbose == true) std::cout << this->bal << std::endl;
            
            this->sum.sell   += raw;
            this->sum.feetax += i.SFee(raw);
            
            if ((o.tck_orig >= -1) && (o.tck_orig < szTck)) {
                int idx = (o.tck_orig != -1 ? idxPs1 - o.tck_orig : idxTckOrigS0);
                this->sum.tck_orig[(std::size_t)idx].bal += raw - i.SFee(raw);
                this->sum.tck_orig[(std::size_t)idx].q   += pq.q;
                this->sum.tck_orig[(std::size_t)idx].evt += 1;
            }
        }
        o.q -= pq.q;
        if (o.q < 0) std::cerr << "OrderBook.ApplyTrade: Negative order q reached" << std::endl;
    }
}

template <class TOrder, class TItem>
void OrderBook<TOrder, TItem>::ApplyCancel(it_itm_t<TItem> iItems, it_ord_t<TOrder> iOrd, INT q)
{
    std::lock_guard<std::recursive_mutex> lock(mutexData);
    
    if (q > 0)
    {
        auto &i = *iItems->second;
        auto &o = iOrd->second;
        if (verbose == true) std::cout << "<Cancel> " << (o.type == kOrdBuy ? "b" : "s") << " {" << iItems->first << "} " << o.p << " (" << o.q << ") [-] (" << q << ") = (" << o.q - q << ")" << std::endl;
        
        if (o.type == kOrdBuy)
        {
            if (verbose == true) std::cout << "    [bal]: " << this->bal << " [+] " << o.p << " * (" << q << ") * (1 + f_b) = ";
            INT64 raw = (INT64)o.p * q;
            this->bal += raw + i.BFee(raw);
            if (verbose == true) std::cout << this->bal << std::endl;
        } else
        if (o.type == kOrdSell)
        {
            if (verbose == true) std::cout << "    [cnt]: {" << iItems->first << "} (" << i.cnt << ") [+] (" << q << ") = ";
            i.cnt += q;
            if (verbose == true) std::cout << i.cnt << std::endl;
        }
        o.q -= q;
        if (o.q < 0) std::cerr << "OrderBook.ApplyCancel: Negative order q reached" << std::endl;
    }
}

}

#endif /* SIBYL_SERVER_ORDERBOOK_H_ */