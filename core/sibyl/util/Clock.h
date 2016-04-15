#ifndef SIBYL_UTIL_CLOCK_H_
#define SIBYL_UTIL_CLOCK_H_

#include <chrono>
#include <sstream>
#include <string>
#include <iomanip>

#include "../sibyl_common.h"

namespace sibyl
{

// Clock representing time point in milliseconds from 00:00:00:000 local time as int 
// system_clock is referenced only once at constructor and the rest uses steady_clock 
class Clock
{
public:
    Clock();
    int Now() const; // milliseconds from 00:00:00:000 local time
    static int   HHMMSS_to_ms(CSTR &str); // pad 0 ms and convert
    static CSTR& ms_to_HHMMSS(int ms);    // discard milliseconds
protected:
    int ms_init;
    std::chrono::steady_clock::time_point stdclk_init;
};

}

#endif /* SIBYL_UTIL_CLOCK_H_ */