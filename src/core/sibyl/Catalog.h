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

// Container for dynamically allocated securities with other relevant info
// Base class for Portfolio and OrderBook
template <class TItem> // default: Item
class Catalog
{
public:
    std::atomic_int time; // [seconds] from 09:00:00 (-kTimeBounds::null)
    INT64           bal;  // balance excluding amount staged as buy orders

    // Accumulated statistics
    struct {
        INT64 buy, sell, feetax;

        // For each tick at which an order was placed
        struct tck_orig_sum {
            INT64 bal, q, evt;
        };

        // Index convention is the same as usual for [0, idx::szTb)
        // Use idxTckOrigS0 & idxTckOrigB0 for s0 & b0 
        std::array<tck_orig_sum, idx::szTb + 2> tck_orig;
    } sum;

    // Pointers to items; indexed by code
    std::map<STR, std::unique_ptr<TItem>> items;

    void   UpdateRefInitBal();
    double GetProfitRate   (bool isRef = false); // based on balInit by default
    
    // Evaluation of the whole catalog
    struct SEval {
        INT64 balU,    // unused balance (i.e., excluding buy orders)
              balBO,   // balance staged as buy orders
              evalCnt, // evaluated sum of cnt's (i.e., excluding sell orders) 
              evalSO,  // evaluated sum of sell orders
              evalTot; // sum of above
    };
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