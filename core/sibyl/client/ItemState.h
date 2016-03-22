#ifndef SIBYL_CLIENT_ITEMSTATE_H_
#define SIBYL_CLIENT_ITEMSTATE_H_

#include <array>

#include "../SibylCommon.h"
#include "../Security_ELW.h"

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
    ItemState() : time(0), pr(0.0f), qr(0), tbr{},
                  isELW(false), iCP(0), expiry(0), kospi200(0.0f), thr{} {}
};

}

#endif /* SIBYL_CLIENT_ITEMSTATE_H_ */