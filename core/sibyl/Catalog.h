#ifndef SIBYL_CATALOG_H_
#define SIBYL_CATALOG_H_

#include <memory>
#include <map>

#include "Security.h"
#include "Participant.h"

namespace sibyl
{
    
template <class TItem> // default: Item
class Catalog          // container for dynamically allocated TItem with other relevant info
{
public:
    int   time;
    INT64 bal;
    struct {
        INT64 buy, sell, feetax;
        struct tck_orig_sum {
            INT64 bal, q, evt;
        };
        std::array<tck_orig_sum, szTb + 2> tck_orig;
    } sum;
    std::map<STR, std::unique_ptr<TItem>> items;

    void   SetTimeBounds   (TimeBounds timeBounds_); // to be called by Participant
    void   UpdateRefInitBal();
    double GetProfitRate   (bool isRef = false); // based on balInit by default
    
    struct SEval { INT64 balU, balBO, evalCnt, evalSO, evalTot; };
    SEval Evaluate() const;
    
    Catalog() : time(TimeBounds::null), bal(0), sum{0, 0, 0, {}},
                balRef(0), balInit(0), timeBounds(TimeBounds::null), isFirstTick(true) {}
protected:
    INT64 balRef;  // evaluation with 'reference price' (= ending price from the previous day)
    INT64 balInit; // evaluation with 'starting price'  (= price right after marken opens)
    
    TimeBounds timeBounds;
    
private:
    bool isFirstTick;
};

template <class TItem>
void Catalog<TItem>::SetTimeBounds(TimeBounds timeBounds_)
{
    timeBounds = timeBounds_;
}

template <class TItem>
void Catalog<TItem>::UpdateRefInitBal()
{
    if ((isFirstTick == true) || (time <= timeBounds.ref) || (time <= timeBounds.init))
    {
        SEval se = Evaluate();
        
        if ((isFirstTick == true) || (time <= timeBounds.ref))
        {
            balRef = se.evalTot;
            isFirstTick = false;
        }
        if (time <= timeBounds.init)
            balInit = se.evalTot;
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
            verify(o.type != kOrdNull);
            if (o.type == kOrdBuy) {
                delta = (INT64)o.p * o.q;
                delta = delta + i.BFee(delta);
                se.balBO += delta; se.evalTot += delta;
            } else
            if (o.type == kOrdSell) {
                delta = (INT64)i.Ps0() * o.q;
                delta = delta - i.SFee(delta);
                se.evalSO += delta; se.evalTot += delta;
            }
        }
    }
    return se;
}

}

#endif /* SIBYL_CATALOG_H_ */