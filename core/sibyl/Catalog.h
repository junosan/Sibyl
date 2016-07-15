/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_CATALOG_H_
#define SIBYL_CATALOG_H_

#include <memory>
#include <map>
#include <atomic>
#include <vector>
#include <algorithm>

#include "Security.h"
#include "time_common.h"

namespace sibyl
{
    
template <class TItem> // default: Item
class Catalog          // container for dynamically allocated TItem with other relevant info
{
public:
    std::atomic_int time;
    INT64 bal;
    struct {
        INT64 buy, sell, feetax;
        struct tck_orig_sum {
            INT64 bal, q, evt;
        };
        std::array<tck_orig_sum, idx::szTb + 2> tck_orig;
    } sum;
    std::map<STR, std::unique_ptr<TItem>> items;

    void   UpdateRefInitBal();
    double GetProfitRate   (bool isRef = false); // based on balInit by default
    
    struct SEval { INT64 balU, balBO, evalCnt, evalSO, evalTot; };
    SEval Evaluate() const;

    // returns (at most) n items sorted by count * price 
    std::vector<typename std::map<STR, std::unique_ptr<TItem>>::const_iterator> GetTopCnts(std::size_t n);
    
    Catalog() : time(kTimeBounds::null), bal(0), sum{0, 0, 0, {}},
                balRef(0), balInit(0), isFirstTick(true) {}
protected:
    INT64 balRef;  // evaluation with 'reference price' (= ending price from the previous day)
    INT64 balInit; // evaluation with 'starting price'  (= price right after marken opens)
    
    constexpr static const int idxTckOrigS0 = idx::szTb + 0;  // not to be used on tbr
    constexpr static const int idxTckOrigB0 = idx::szTb + 1;  // not to be used on tbr
private:
    bool isFirstTick;
};

template <class TItem>
void Catalog<TItem>::UpdateRefInitBal()
{
    if (isFirstTick == true || time <=  kTimeBounds::ref || time <=  kTimeBounds::init)
    {
        SEval se = Evaluate();
        
        if (isFirstTick == true)
        {
            balRef  = se.evalTot;
            balInit = se.evalTot;
            isFirstTick = false;
        }
        else
        {
            if (time <=  kTimeBounds::ref ) balRef  = se.evalTot;
            if (time <=  kTimeBounds::init) balInit = se.evalTot;
        }
    }
}

template <class TItem>
double Catalog<TItem>::GetProfitRate(bool isRef)
{
    SEval se = Evaluate();
    return (isRef == true ? (double) se.evalTot / balRef : (double) se.evalTot / balInit);
}

template <class TItem>
typename Catalog<TItem>::SEval Catalog<TItem>::Evaluate() const {
    SEval se;
    se.balU  = se.evalTot = bal;
    se.balBO = se.evalCnt = se.evalSO = 0;
    for (const auto &code_pItem : items)
    {
        auto &i = *code_pItem.second;
        INT64 delta = (INT64)i.Ps0() * i.cnt;
        delta = delta - i.SFee(delta);
        se.evalCnt += delta; se.evalTot += delta;
        for (const auto &price_TOrder : i.ord)
        {
            const auto &o = price_TOrder.second;
            verify(o.type != OrdType::null);
            if (o.type == OrdType::buy) {
                delta = (INT64)o.p * o.q;
                delta = delta + i.BFee(delta);
                se.balBO += delta; se.evalTot += delta;
            } else
            if (o.type == OrdType::sell) {
                delta = (INT64)i.Ps0() * o.q;
                delta = delta - i.SFee(delta);
                se.evalSO += delta; se.evalTot += delta;
            }
        }
    }
    return se;
}

template <class TItem>
std::vector<typename std::map<STR, std::unique_ptr<TItem>>::const_iterator> Catalog<TItem>::GetTopCnts(std::size_t n)
{
    verify(n >= 0);

    using it_t = typename std::map<STR, std::unique_ptr<TItem>>::const_iterator; 
    std::vector<it_t> vec;
    for (auto it = std::begin(items), end = std::end(items); it != end; ++it)
        if (it->second->cnt > 0) vec.push_back(it);
    
    n = std::min(n, vec.size());
    std::partial_sort(std::begin(vec), std::begin(vec) + n, std::end(vec), [](it_t a, it_t b){
        return (INT64) a->second->cnt * a->second->Ps0() > (INT64) b->second->cnt * b->second->Ps0(); });
    
    vec.resize(n);
    return vec; // relies on RVO
}

}

#endif /* SIBYL_CATALOG_H_ */