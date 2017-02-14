/*
   Copyright 2017 Hosang Yoon

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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