#ifndef SIBYL_SECURITY_H_
#define SIBYL_SECURITY_H_

#include <string>
#include <array>
#include <map>
#include <algorithm>
#include <cassert>

#include "sibyl_common.h"

namespace sibyl
{

namespace idx
{
    constexpr int tckN = 10; // largest tick 
    constexpr int szTb = tckN * 2;
    constexpr int ps1  = tckN - 1;
    constexpr int pb1  = tckN;
}

enum class SecType { null, KOSPI, ELW, ETF };
enum class OrdType { null, buy, sell       };

class PQ
{
public:
    INT p, q;
    PQ()               : p(0 ), q(0 ) {}
    PQ(int p_, int q_) : p(p_), q(q_) {}
};

template <class TOrder> // default: Order
class Security          // abstract class; derive first as Item with application specific members, and then derive it as KOSPI/ELW/etc.
{
public:
    FLOAT                      pr;
    INT64                      qr;
    std::array<PQ, idx::szTb>  tbr;
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
    
    INT Ps0() const { return TckLo(tbr[idx::ps1].p); }
    INT Pb0() const { return TckHi(tbr[idx::pb1].p); }
    
    void Requantize(std::array<PQ, idx::szTb> &in, INT trPs1, INT trPb1);
    void Requantize(INT trPs1, INT trPb1)          { Requantize(tbr, trPs1          , trPb1          ); }
    void Requantize(std::array<PQ, idx::szTb> &in) { Requantize(in , in [idx::ps1].p, in [idx::pb1].p); }
    void Requantize()                              { Requantize(tbr, tbr[idx::ps1].p, tbr[idx::pb1].p); }                                               
    
    // Utility functions for OrdType-generic operations
    // NOTE: the following must be used on a Requantized tbr
    // tck: -1 (ps0/pb0), 0 (0-based tick), idx::tckN (not found) 
    int P2Tck(INT p  , OrdType type) const;
    INT Tck2P(int tck, OrdType type) const;
    INT Tck2Q(int tck, OrdType type) const;
    
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

}

#include "securities/Security_KOSPI.h"
#include "securities/Security_ELW.h"
#include "securities/Security_ETF.h"

#endif /* SIBYL_SECURITY_H_ */