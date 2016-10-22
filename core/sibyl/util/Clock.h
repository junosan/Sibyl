/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_UTIL_CLOCK_H_
#define SIBYL_UTIL_CLOCK_H_

#include <chrono>
#include <string>

#include "../sibyl_common.h"

namespace sibyl
{

// Clock representing time point in milliseconds from 00:00:00:000 local time as int 
// system_clock is referenced only once at constructor and the rest uses steady_clock
// Use the global instance "clock" 
class Clock
{
public:
    Clock();
    int Now() const; // milliseconds from 00:00:00:000 local time
    static int   HHMMSS_to_ms(CSTR &str); // pad 0 ms and convert
    static CSTR& ms_to_HHMMSS(int ms, bool colons = false); // discards trailing milliseconds
protected:
    int ms_init;
    std::chrono::steady_clock::time_point stdclk_init;
};

extern Clock clock;

}

#endif /* SIBYL_UTIL_CLOCK_H_ */