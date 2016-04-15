#ifndef SIBYL_PARTICIPANT_H_
#define SIBYL_PARTICIPANT_H_

#include "SibylCommon.h"

namespace sibyl
{

class TimeBounds
{
public:
    constexpr static const int null = -9 * 3600; // this is also used to reconstruct 00:00:00-based time
    int ref, init, stop, end;
    TimeBounds(int t)                      : ref(t), init(t), stop(t), end(t) {}
    TimeBounds(int r, int i, int s, int e) : ref(r), init(i), stop(s), end(e) {}
};

class Participant
{
public:
    TimeBounds timeBounds;
    Participant() : timeBounds(       -3600 /* ref  */,
                               kTimeTickSec /* init */,
                                      21000 /* stop */,
                                      22200 /* end  */) {}
};

}

#endif /* SIBYL_PARTICIPANT_H_ */