#ifndef SIBYL_TIME_COMMON_H_
#define SIBYL_TIME_COMMON_H_

namespace sibyl
{

namespace kTimeRates
{
    constexpr int secPerTick = 10;
    constexpr int reqPerSec  = 5;
    constexpr int reqPerTick = reqPerSec * secPerTick;
}

namespace kTimeBounds
{
    constexpr int null =              -9 * 3600; // defines conversion to 00:00:00-based time
    constexpr int ref  =                  -3600; // 08:00:00 last ref price reference
    constexpr int init = kTimeRates::secPerTick; // 09:00:10 initiate rnn
    constexpr int stop =                  21000; // 14:50:00 stop rnn
    constexpr int end  =                  22200; // 15:10:00 end everything
}

}

#endif /* SIBYL_TIME_COMMON_H_ */