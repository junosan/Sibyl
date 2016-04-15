#ifndef SIBYL_TIMEBOUNDS_H_
#define SIBYL_TIMEBOUNDS_H_

#include "sibyl_common.h"

namespace sibyl
{

constexpr int kTimeTickSec = 10;
constexpr int kReqNPerSec  = 5;
constexpr int kReqNPerTick = kReqNPerSec * kTimeTickSec;

class TimeBounds
{
public:
    constexpr static int null =    -9 * 3600; // defines conversion to 00:00:00-based time
    constexpr static int ref  =        -3600;
    constexpr static int init = kTimeTickSec;
    constexpr static int stop =        21000;
    constexpr static int end  =        22200;
};

}

#endif /* SIBYL_TIMEBOUNDS_H_ */