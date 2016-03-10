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
    ItemState() : time(0), pr(0.0f), qr(0), tbr{} {}
};

}

#endif /* SIBYL_CLIENT_ITEMSTATE_H_ */