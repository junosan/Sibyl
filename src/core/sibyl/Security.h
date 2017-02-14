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

#ifndef SIBYL_SECURITY_H_
#define SIBYL_SECURITY_H_

#include <string>
#include <array>
#include <map>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <cmath>

#include "sibyl_common.h"

namespace sibyl
{

// Index convention for prices/quants stored in Security::tbr
// Prices are stored in two consecutive sections {ps10-ps1}{pb1-pb10} where
//     ps1: 1 tick above the price of immediate sell, ps0 (= highest bid in the market)
//     pb1: 1 tick below the price of immediate buy,  pb0 (= lowest  ask in the market)
// If there is no gap between bids/asks, the prices are sorted from highest to lowest
// If there is a gap, some prices at lower ticks may overlap
namespace idx 
{
    constexpr int tckN = 10; // largest tick 
    constexpr int szTb = tckN * 2;
    constexpr int ps1  = tckN - 1;
    constexpr int pb1  = tckN;
}

enum class SecType
{
    null,
    KOSPI,
    ELW,
    ETF
};

enum class OrdType
{
    null =  0,
    buy  = +1, // a.k.a. bid
    sell = -1  // a.k.a. ask
};

inline std::ostream& operator<<(std::ostream &os, OrdType type)
{
    switch (type)
    {
        case OrdType::null : return os << "null";
        case OrdType::buy  : return os << "b";
        case OrdType::sell : return os << "s";   
    }
    return os;
}

class PQ
{
public:
    INT p, q;
    PQ()               : p(0 ), q(0 ) {}
    PQ(int p_, int q_) : p(p_), q(q_) {}
};

// Abstract class for holding the current state of a security item
// Derive with application specific members, then derive again as KOSPI/ELW/etc.
template <class TOrder> // TOrder should be derived from PQ with additional members as needed
class Security
{
public:
    FLOAT                      pr;  // average trade price     during        this time tick
    INT64                      qr;  // trade amount            during        this time tick
    std::array<PQ, idx::szTb>  tbr; // bids/asks in the market at the end of this time tick
    INT                        cnt; // # of idle holds that I own, NOT staged as sell orders
    std::multimap<INT, TOrder> ord; // orders placed by me staged in the market; indexed by price
    
    virtual SecType Type  ()        const = 0;
    virtual INT     TckHi (INT p)   const = 0; // price one tick higher than p 
    virtual INT     TckLo (INT p)   const = 0; // price one tick lower  than p
    virtual bool    ValidP(INT p)   const = 0; // check if p conforms to tick rules of a security
    virtual INT64   BFee  (INT64 r) const = 0; // floored/rounded (fee + tax) for raw buy  price * quant
    virtual INT64   SFee  (INT64 r) const = 0; // floored/rounded (fee + tax) for raw sell price * quant
    virtual double  dBF   ()        const = 0; // ratio of buy  (fee + tax)
    virtual double  dSF   ()        const = 0; // ratio of sell (fee + tax)
    
    // NOTE: the following must be used on Requantized tbr
    INT Ps0() const { return TckLo(tbr[idx::ps1].p); }
    INT Pb0() const { return TckHi(tbr[idx::pb1].p); }
    
    // Convert from raw market data to ps#|pb# convention defined above, while filling any price gaps 
    void Requantize(std::array<PQ, idx::szTb> &in, INT trPs1, INT trPb1);
    void Requantize(INT trPs1, INT trPb1)          { Requantize(tbr, trPs1          , trPb1          ); }
    void Requantize(std::array<PQ, idx::szTb> &in) { Requantize(in , in [idx::ps1].p, in [idx::pb1].p); }
    void Requantize()                              { Requantize(tbr, tbr[idx::ps1].p, tbr[idx::pb1].p); }                                               
    
    // Utility functions for OrdType-generic operations
    // NOTE: the following must be used on Requantized tbr
    // tck: -1 (ps0/pb0), 0 (ps1/pb1), ..., idx::tckN (not found) 
    int P2Tck(INT p  , OrdType type) const;
    INT Tck2P(int tck, OrdType type) const;
    INT Tck2Q(int tck, OrdType type) const;
    
    // Max quantity that can be bought for given balance and price, i.e., floor(bal / (p + BFee)) 
    // Handling potential overflow & special cases (e.g., ELW)
    INT MaxBuyQ(INT64 bal, INT p) const;
    
    Security() : pr(0.0f), qr(0), cnt(0) {}
    virtual ~Security() {}
};

template <class TOrder>
void Security<TOrder>::Requantize(std::array<PQ, idx::szTb> &in, INT trPs1, INT trPb1)
{
    if ((trPs1 <= 0) && (trPb1 <= 0)) { trPs1 = in[idx::ps1].p; trPb1 = in[idx::pb1].p; }
    if ((trPs1 <= 0) && (trPb1 <= 0))                     return;
    if ((trPs1 >  0) && (trPb1 >  0) && (trPs1 <= trPb1)) return; // triggered by unknown invalid situation in table values; only happens when trPs1 & trPb1 are pulled from tbp (t < 0) and is safe to ignore
    std::array<PQ, idx::szTb> out;   
    out[idx::ps1].p = (trPb1 > 0 ? TckHi(trPb1) : trPs1);
    out[idx::pb1].p = (trPs1 > 0 ? TckLo(trPs1) : trPb1);
    for (auto iO = std::begin(out) + idx::ps1; iO > std::begin(out)  ; iO--) (iO - 1)->p = TckHi(iO->p);
    for (auto iO = std::begin(out) + idx::pb1; iO < std::end(out) - 1; iO++) (iO + 1)->p = TckLo(iO->p);
    for (auto &o : out) {
        auto iI = std::find_if(std::begin(in), std::end(in), [&o](const PQ &i) { return i.p == o.p; });
        o.q = (iI != std::end(in) ? iI->q : 0);        
    }
    in = out;
}

template <class TOrder>
int Security<TOrder>::P2Tck(INT p, OrdType type) const
{
    int tck = (int)idx::tckN;
    if (type == OrdType::sell) {
        if (p == Ps0()) tck = -1;
        else { // find_if [first, last)
            auto first = std::begin(tbr);
            auto last  = std::begin(tbr) + idx::ps1 + 1;
            auto iT    = std::find_if(first, last, [&p](const PQ &tb) { return tb.p == p; });
            if (iT != last) tck = (int)idx::ps1 - (int)(iT - first);
        }
    } else
    if (type == OrdType::buy ) {
        if (p == Pb0()) tck = -1;
        else { // find_if [first, last)
            auto first = std::begin(tbr) + idx::pb1;
            auto last  = std::end(tbr);
            auto iT    = std::find_if(first, last, [&p](const PQ &tb) { return tb.p == p; });
            if (iT != last) tck = (int)(iT - first);
        }
    }
    return tck;
}

template <class TOrder>
INT Security<TOrder>::Tck2P(int tck, OrdType type) const
{
    verify((tck >= -1) && (tck < (int)idx::tckN));
    if (type == OrdType::sell) {
        if (tck == -1) return Ps0();
        else           return tbr[(std::size_t)(idx::ps1 - tck)].p;
    } else
    if (type == OrdType::buy ) {
        if (tck == -1) return Pb0();
        else           return tbr[(std::size_t)(idx::pb1 + tck)].p;
    }
    verify(false);
}

template <class TOrder>
INT Security<TOrder>::Tck2Q(int tck, OrdType type) const
{
    verify((tck >= -1) && (tck < (int)idx::tckN));
    if (tck >=  0) {
        if      (type == OrdType::sell) return tbr[(std::size_t)(idx::ps1 - tck)].q;
        else if (type == OrdType::buy ) return tbr[(std::size_t)(idx::pb1 + tck)].q;
    } else
    if (tck == -1) { // not to be used unless for depletion mechanism in Simulation
        INT p = Tck2P(-1, type);
        INT q = 0;
        for (const auto &tb : tbr) if (p == tb.p) { q = tb.q; break; }
        return q;
    }
    verify(false);
}

#include "util/toggle_win32_min_max.h"

template <class TOrder>
INT Security<TOrder>::MaxBuyQ(INT64 bal, INT p) const
{
    INT64 qmax64 = (INT64)std::floor(bal / (p * (1.0 + dBF())));
    INT   qmax   =   (INT)std::min(qmax64, (INT64) std::numeric_limits<INT>::max());
    if (Type() == SecType::ELW) qmax -= qmax % 10;
    return qmax;
}

#include "util/toggle_win32_min_max.h"

}

#include "securities/Security_KOSPI.h"
#include "securities/Security_ELW.h"
#include "securities/Security_ETF.h"

#endif /* SIBYL_SECURITY_H_ */