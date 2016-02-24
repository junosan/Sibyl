#ifndef SIBYL_CATALOG_H_
#define SIBYL_CATALOG_H_

#include <map>

#include "Security.h"

namespace sibyl
{
    
template <class TItem> // default: Item
class Catalog          // container for dynamically allocated TItem with other relevant info
{
public:
    int   time;
    INT64 bal;
    struct {
        INT64 buy;
        INT64 sell;
        INT64 feetax;
    } sum;
    std::map<std::string, std::unique_ptr<TItem>> p;

    void SetTimeBoundaries(int ref, int init, int stop, int end) {
        timeRef = ref; timeInit = init; timeStop = stop; timeEnd = end;
    }
    void UpdateRefInitBal ();

    struct SEval { INT64 balU, balBO, evalCnt, evalSO, evalTot; };
    SEval Evaluate() const;
    
    constexpr static const int timeDefault = -9 * 3600;
    Catalog() : time(timeDefault), bal(0), sum{0, 0, 0},
                isFirstTick(true), balRef(0), balInit(0),
                timeRef (timeDefault), 
                timeInit(timeDefault),
                timeStop(timeDefault),
                timeEnd (timeDefault) {}
protected:
    bool isFirstTick;
    INT64 balRef;  // evaluation with 'reference price' (= ending price from the previous day)
    INT64 balInit; // evaluation with 'starting price'  (= price right after marken opens)
    
    int timeRef;   // (08:00:00) last ref price reference
    int timeInit;  // (09:00:10) start input to rnn
    int timeStop;  // (14:50:00) stop input to rnn
    int timeEnd;   // (15:10:00) terminate
};

template <class TItem>
void Catalog<TItem>::UpdateRefInitBal()
{
    if ((isFirstTick == true) || (time <= timeRef) || (time <= timeInit))
    {
        SEval se = Evaluate();
        
        if ((isFirstTick == true) || (time <= timeRef))
        {
            balRef = se.evalTot;
            isFirstTick = false;
        }
        if (time <= timeInit)
            balInit = se.evalTot;
    }
}

template <class TItem>
struct Catalog<TItem>::SEval Catalog<TItem>::Evaluate() const {
    SEval se;
    se.balU  = se.evalTot = bal;
    se.balBO = se.evalCnt = se.evalSO = 0;
    for (auto iP = std::begin(p); iP != std::end(p); iP++)
    {
        auto &i = *(iP->second);
        INT64 delta = (INT64)i.Ps0() * i.cnt;
        delta = delta - i.SFee(delta);
        se.evalCnt += delta; se.evalTot += delta;
        for (auto iO = std::begin(i.ord); iO != std::end(i.ord); iO++) {
            assert(iO->type != kOrdNull);
            if (iO->type == kOrdBuy) {
                delta = (INT64)iO->p * iO->q;
                delta = delta + i.BFee(delta);
                se.balBO += delta; se.evalTot += delta;
            } else
            if (iO->type == kOrdSell) {
                delta = (INT64)i.Ps0() * iO->q;
                delta = delta - i.SFee(delta);
                se.evalSO += delta; se.evalTot += delta;
            }
        }
    }
    return se;
}

}

#endif /* SIBYL_CATALOG_H_ */