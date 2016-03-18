#ifndef SIBYL_SECURITY_H_
#define SIBYL_SECURITY_H_

#include <string>
#include <array>
#include <map>
#include <algorithm>
#include <cassert>

#include "SibylCommon.h"

namespace sibyl
{

template <class TOrder> // default: Order
class Security          // abstract class; derive first as Item with application specific members, and then derive it as KOSPI/ELW/etc.
{
public:
    FLOAT                      pr;
    INT64                      qr;
    std::array<PQ, szTb>       tbr;
    INT                        cnt;
    std::multimap<INT, TOrder> ord;
    
    virtual SecType Type  ()        const = 0;
    virtual INT     TckHi (INT p)   const = 0;
    virtual INT     TckLo (INT p)   const = 0;
    virtual bool    ValidP(INT p)   const = 0;
    virtual INT64   BFee  (INT64 r) const = 0;
    virtual INT64   SFee  (INT64 r) const = 0;
    virtual double  dBF   ()        const = 0;
    virtual double  dSF   ()        const = 0;
    
    INT Ps0() const { return TckLo(tbr[idxPs1].p); }
    INT Pb0() const { return TckHi(tbr[idxPb1].p); }
    
    void Requantize(std::array<PQ, szTb> &in, INT trPs1, INT trPb1);
    void Requantize(INT trPs1, INT trPb1)     { Requantize(tbr, trPs1        , trPb1        ); }
    void Requantize(std::array<PQ, szTb> &in) { Requantize(in , in [idxPs1].p, in [idxPb1].p); }
    void Requantize()                         { Requantize(tbr, tbr[idxPs1].p, tbr[idxPb1].p); }                                               
    
    // Utility functions for OrdType-generic operations
    // NOTE: the following must be used on a Requantized tbr
    // tck: -1 (ps0/pb0), 0 (0-based tick), szTck (not found) 
    int P2Tck(INT p  , OrdType type) const;
    INT Tck2P(int tck, OrdType type) const;
    INT Tck2Q(int tck, OrdType type) const;
    
    Security() : pr(0.0f), qr(0), cnt(0) {}
    virtual ~Security() {}
};

template <class TOrder>
void Security<TOrder>::Requantize(std::array<PQ, szTb> &in, INT trPs1, INT trPb1)
{
    if ((trPs1 <= 0) && (trPb1 <= 0)) { trPs1 = in[idxPs1].p; trPb1 = in[idxPb1].p; }
    if ((trPs1 <= 0) && (trPb1 <= 0))                     return;
    if ((trPs1 >  0) && (trPb1 >  0) && (trPs1 <= trPb1)) return; // triggered by unknown invalid situation in table values; only happens when trPs1 & trPb1 are pulled from tbp (t < 0) and is safe to ignore
    std::array<PQ, szTb> out;   
    out[idxPs1].p = (trPb1 > 0 ? TckHi(trPb1) : trPs1);
    out[idxPb1].p = (trPs1 > 0 ? TckLo(trPs1) : trPb1);
    for (auto iO = std::begin(out) + idxPs1; iO > std::begin(out)  ; iO--) (iO - 1)->p = TckHi(iO->p);
    for (auto iO = std::begin(out) + idxPb1; iO < std::end(out) - 1; iO++) (iO + 1)->p = TckLo(iO->p);
    for (auto &o : out) {
        auto iI = std::find_if(std::begin(in), std::end(in), [&o](const PQ &i) { return i.p == o.p; });
        o.q = (iI != std::end(in) ? iI->q : 0);        
    }
    in = out;
}

template <class TOrder>
int Security<TOrder>::P2Tck(INT p, OrdType type) const
{
    int tck = (int)szTck;
    if (type == kOrdSell) {
        if (p == Ps0()) tck = -1;
        else { // find_if [first, last)
            auto first = std::begin(tbr);
            auto last  = std::begin(tbr) + idxPs1 + 1;
            auto iT    = std::find_if(first, last, [&p](const PQ &tb) { return tb.p == p; });
            if (iT != last) tck = (int)idxPs1 - (int)(iT - first);
        }
    } else
    if (type == kOrdBuy ) {
        if (p == Pb0()) tck = -1;
        else { // find_if [first, last)
            auto first = std::begin(tbr) + idxPb1;
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
    assert((tck >= -1) && (tck < (int)szTck));
    if (type == kOrdSell) {
        if (tck == -1) return Ps0();
        else           return tbr[(std::size_t)(idxPs1 - tck)].p;
    } else
    if (type == kOrdBuy ) {
        if (tck == -1) return Pb0();
        else           return tbr[(std::size_t)(idxPb1 + tck)].p;
    }
    assert(false);
}

template <class TOrder>
INT Security<TOrder>::Tck2Q(int tck, OrdType type) const
{
    assert((tck >= -1) && (tck < (int)szTck));
    if (tck >=  0) {
        if      (type == kOrdSell) return tbr[(std::size_t)(idxPs1 - tck)].q;
        else if (type == kOrdBuy ) return tbr[(std::size_t)(idxPb1 + tck)].q;
    } else
    if (tck == -1) { // not to be used unless for depletion mechanism in Simulation
        INT p = Tck2P(-1, type);
        INT q = 0;
        for (const auto &tb : tbr) if (p == tb.p) { q = tb.q; break; }
        return q;
    }
    assert(false);
}

}

#endif /* SIBYL_SECURITY_H_ */