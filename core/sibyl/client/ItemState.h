#ifndef SIBYL_CLIENT_ITEMSTATE_H_
#define SIBYL_CLIENT_ITEMSTATE_H_

#include <array>

#include "../sibyl_common.h"
#include "../Security.h"

namespace sibyl
{

/* For communication with fractal */
class ItemState
{
public:
    // common
    STR   code;
    int   time;
    FLOAT pr;
    INT64 qr;
    std::array<PQ, szTb> tbr;
    
    // ELW only
    bool isELW;
    int iCP; // +1 for call, -1 for put
    int expiry;
    FLOAT kospi200;
    std::array<FLOAT, ELW<Security<PQ>>::szTh> thr;
    
    // ETF only
    bool isETF;
    FLOAT devNAV;
    
    ItemState() : time(0), pr(0.0f), qr(0), tbr{},
                  isELW(false), iCP(0), expiry(0), kospi200(0.0f), thr{},
                  isETF(false), devNAV(0.0f)
                  {}
};

}

#endif /* SIBYL_CLIENT_ITEMSTATE_H_ */