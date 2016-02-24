#ifndef SIBYL_SECURITY_H_
#define SIBYL_SECURITY_H_

#include <string>
#include <array>
#include <list>
#include <algorithm>
#include <vector>
#include <memory>
#include <cmath>
#include <cassert>

#include "SibylCommon.h"

namespace sibyl
{

class PQ // derive first as Order with application specific members (e.g., order type, order number, original tick, etc.)
{
public:
    INT p;
    INT q;
    PQ() : p(0), q(0) {}    
};

template <class TOrd> // Portfolio: std::vector<Order>, OrderBook: std::multimap<INT, Order> 
class Security        // abstract class; derive first as Item with application specific members, and then derive it as Stock/ELW/etc.
{
public:
    FLOAT                pr;
    INT64                qr;
    std::array<PQ, szTb> tbr;
    INT                  cnt;
    TOrd                 ord;
    
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
    
    void Requantize(std::array<PQ, szTb> &in, INT trPs1, INT trPb1) {
        if ((trPs1 <= 0) && (trPb1 <= 0))                     return;
        if ((trPs1 >  0) && (trPb1 >  0) && (trPs1 <= trPb1)) return; // triggered by unknown invalid situation in table values; only happens when trPs1 & trPb1 are pulled from tbp (t < 0) and is safe to ignore
        std::array<PQ, szTb> out;   
        out[idxPs1].p = (trPb1 > 0 ? TckHi(trPb1) : trPs1);
        out[idxPb1].p = (trPs1 > 0 ? TckLo(trPs1) : trPb1);
        for (auto iO = std::begin(out) + idxPs1; iO > std::begin(out)  ; iO--) (iO - 1)->p = TckHi(iO->p);
        for (auto iO = std::begin(out) + idxPb1; iO < std::end(out) - 1; iO++) (iO + 1)->p = TckLo(iO->p);
        std::for_each(std::begin(out), std::end(out), [&in](PQ &o) {
            auto iI = std::find_if(std::begin(in), std::end(in), [&o](const PQ &i) { return i.p == o.p; });
            o.q = (iI != std::end(in) ? iI->q : 0);        
        });
        in = out;
    }
    void Requantize(INT trPs1, INT trPb1) { Requantize(tbr, trPs1, trPb1); }
    
    // Utility functions for OrdType-generic operations
    // NOTE: the following must be used on a Requantized tbr
    // tck: -1 (ps0/pb0), 0 (0-based tick), szTck (not found) 
    int P2Tck(const INT &p  , const OrdType &type) const {
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
    INT Tck2P(const int &tck, const OrdType &type) const {
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
    
    Security() : pr(0.0f), qr(0), cnt(0) {}
    virtual ~Security() {}
};

template <class TItem>     
class Stock : public TItem // derive from a specialized Security<container<TOrd>> (i.e., Item)
{
public:
    // virtuals from Security
    SecType Type  ()        const { return kSecStock;                      }
    INT     TckHi (INT p)   const { 
        return  p + (                 (p <    1000) *      1 +
                      (p >=   1000) * (p <    5000) *      5 +
                      (p >=   5000) * (p <   10000) *     10 +
                      (p >=  10000) * (p <   50000) *     50 +
                      (p >=  50000) * (p <  100000) *    100 +
                      (p >= 100000) * (p <  500000) *    500 +
                      (p >= 500000)                 *   1000 );            }
    INT     TckLo (INT p)   const {
        return (p - (                 (p <=   1000) *      1 +
                      (p >    1000) * (p <=   5000) *      5 +
                      (p >    5000) * (p <=  10000) *     10 +
                      (p >   10000) * (p <=  50000) *     50 +
                      (p >   50000) * (p <= 100000) *    100 +
                      (p >  100000) * (p <= 500000) *    500 +
                      (p >  500000)                 *   1000 )) * (p > 0); }
    bool    ValidP(INT p)   const {
        INT q =                       (p <    1000) *      1 +
                      (p >=   1000) * (p <    5000) *      5 +
                      (p >=   5000) * (p <   10000) *     10 +
                      (p >=  10000) * (p <   50000) *     50 +
                      (p >=  50000) * (p <  100000) *    100 +
                      (p >= 100000) * (p <  500000) *    500 +
                      (p >= 500000)                 *   1000 ;
        INT b =                       (p <    1000) *      0 +
                      (p >=   1000) * (p <    5000) *   1000 +
                      (p >=   5000) * (p <   10000) *   5000 +
                      (p >=  10000) * (p <   50000) *  10000 +
                      (p >=  50000) * (p <  100000) *  50000 +
                      (p >= 100000) * (p <  500000) * 100000 +
                      (p >= 500000)                 * 500000 ;
        return (p > 0) && (((p - b) % q) == 0);                            }
    INT64   BFee  (INT64 r) const { return (INT64)std::round(r * dBF0);    }
    INT64   SFee  (INT64 r) const { return (INT64)std::round(r * dSF0) +
                                           (INT64)std::floor(r * dST0) +
                                           (INT64)std::floor(r * dST1);    }
    double  dBF   ()        const { return dBF0;                           }
    double  dSF   ()        const { return dSF0 + dST0 + dST1;             }
private:
    constexpr static const double dBF0 = 0.00015;
    constexpr static const double dSF0 = 0.00015;
    constexpr static const double dST0 = 0.0015;
    constexpr static const double dST1 = 0.0015;
};

template <class TItem>
class ELW : public TItem{
public:
    // virtuals from Security
    SecType Type  ()        const { return kSecELW;                     }
    INT     TckHi (INT p)   const { return p + 5;                       }
    INT     TckLo (INT p)   const { return (p - 5) * (p > 0);           }
    bool    ValidP(INT p)   const { return (p > 0) && ((p % 5) == 0);   }
    INT64   BFee  (INT64 r) const { return (INT64)std::round(r * dBF0); }
    INT64   SFee  (INT64 r) const { return (INT64)std::round(r * dSF0); }
    double  dBF   ()        const { return dBF0;                        }
    double  dSF   ()        const { return dSF0;                        }
    
    // additionals
    static FLOAT                          kospi200;
    constexpr static const std::ptrdiff_t szTh = 8;  // number of fields in theoretical Greeks
    std::array<FLOAT, szTh>               thr;
    void    SetInfo(OptType optType_, INT expiry_) {
        assert(((optType_ == kOptCall) || (optType_ == kOptPut)) && (expiry_ >= 0));
        optType = optType_; expiry = expiry_;
    }
    OptType CallPut() const { assert(optType != kOptNull); return optType; }
    INT     Expiry () const { assert(optType != kOptNull); return expiry;  }
    ELW() : thr{}, optType(kOptNull), expiry(0) {}
private:
    constexpr static const double dBF0 = 0.00015;
    constexpr static const double dSF0 = 0.00015;
    OptType optType;
    INT     expiry;
};
template <class TItem>
FLOAT ELW<TItem>::kospi200 = 0.0f;

}

#endif /* SIBYL_SECURITY_H_ */