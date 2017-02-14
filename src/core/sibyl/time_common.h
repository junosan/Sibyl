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

#ifndef SIBYL_TIME_COMMON_H_
#define SIBYL_TIME_COMMON_H_

namespace sibyl
{

namespace kTimeRates
{
    constexpr int secPerTick = 10;
    constexpr int reqPerSec  = 5;                      // as required by Kiwoom  
    constexpr int reqPerTick = reqPerSec * secPerTick; // maximum number of reqs possible in a tick
}

namespace kTimeBounds // [seconds]
{
    constexpr int null =              -9 * 3600; // defines conversion to 00:00:00-based time
    constexpr int ref  =                  -3600; // 08:00:00 last ref price reference
    constexpr int init = kTimeRates::secPerTick; // 09:00:10 initiate rnn
    constexpr int stop =                  21000; // 21000 - 14:50:00, 22800 - 15:20:00 stop rnn
    constexpr int end  =                  24000; // 15:40:00 end everything
}

}

#endif /* SIBYL_TIME_COMMON_H_ */