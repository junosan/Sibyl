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

#include "Clock.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace sibyl
{

// The one and only Clock instance
Clock clock;

Clock::Clock()
{
    stdclk_init = std::chrono::steady_clock::now();
    
    auto now = std::chrono::system_clock::now();
    
    // retrieve UTC time zone
    auto now_c = std::chrono::system_clock::to_time_t(now);
    
    int utc_raw;
    std::stringstream ssUTC;
#ifndef __linux__
    ssUTC << std::put_time(std::localtime(&now_c), "%z"); // e.g., +0900 for KST
#else // g++ does not have put_time (to be added in g++ 5.0)
    constexpr std::size_t sz = 1 << 4;
    char str[sz];
    verify(std::strftime(str, sz, "%z", std::localtime(&now_c)) > 0);
    ssUTC << str;
#endif /* !__linux__ */
    ssUTC >> utc_raw;
    verify(ssUTC.fail() == false);
    
    int sign = (utc_raw > 0) - (utc_raw < 0);
    utc_raw *= sign;
    int utc_h = sign * (utc_raw / 100);
    int utc_m = sign * (utc_raw % 100);
    
    // calculate current local time in milliseconds
    auto total_d = now.time_since_epoch() + std::chrono::hours(utc_h) + std::chrono::minutes(utc_m);

    auto days_d  = std::chrono::duration_cast<std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>::type>>(total_d);
        total_d -= days_d;
    // auto hours_d = std::chrono::duration_cast<std::chrono::hours>(total_d);
    //     total_d -= hours_d;
    // auto minutes_d = std::chrono::duration_cast<std::chrono::minutes>(total_d);
    //     total_d -= minutes_d;
    // auto seconds_d = std::chrono::duration_cast<std::chrono::seconds>(total_d);
    //     total_d -= seconds_d;
    auto milliseconds_d = std::chrono::duration_cast<std::chrono::milliseconds>(total_d);

    ms_init = (int) milliseconds_d.count();
}

int Clock::Now() const
{
    return ms_init + (int) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - stdclk_init).count());
}

// static
int Clock::HHMMSS_to_ms(CSTR &str)
{
    verify(str.size() == 6);
    verify(true == std::all_of(std::begin(str), std::end(str), (int (*)(int))std::isdigit));
    int txt = std::stoi(str);
    int h = txt / 10000;
    verify(h < 24);
    int m = (txt - h * 10000) / 100;
    verify(m < 60); 
    int s = txt - h * 10000 - m * 100;
    verify(s < 60);
    return (h * 3600 + m * 60 + s) * 1000;
}

// static
CSTR& Clock::ms_to_HHMMSS(int ms, bool colons)
{
    static STR str;
    verify(ms >= 0 && ms < 24 * 3600 * 1000);
    int tot = ms / 1000;
    int h = tot / 3600;
    int m = (tot - h * 3600) / 60;
    int s = (tot - h * 3600 - m * 60);    
    std::stringstream ss;
    if (colons == false)
    {
        ss << std::setfill('0')
           << std::setw(2) << h
           << std::setw(2) << m
           << std::setw(2) << s;
    }
    else
    {
        ss << std::setfill('0')
           << std::setw(2) << h << ':'
           << std::setw(2) << m << ':'
           << std::setw(2) << s;
    }
    ss >> str;
    return str; 
}

}
