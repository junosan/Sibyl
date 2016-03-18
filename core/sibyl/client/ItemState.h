#ifndef SIBYL_CLIENT_ITEMSTATE_H_
#define SIBYL_CLIENT_ITEMSTATE_H_

#include <array>

#include "../SibylCommon.h"

namespace sibyl
{

/* For communication with fractal */
class ItemState
{
public:
    STR   code;
    int   time;
    FLOAT pr;
    INT64 qr;
    std::array<PQ, szTb> tbr;
    // bool isELW;
    // FLOAT kospi200;
    // std::array<FLOAT, szTh> thr;
    ItemState() : time(0), pr(0.0f), qr(0), tbr{}/*,
                  isELW(false), kospi200(0.0f), thr{} */ {}
};

}

#endif /* SIBYL_CLIENT_ITEMSTATE_H_ */