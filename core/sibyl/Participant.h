#ifndef SIBYL_PARTICIPANT_H_
#define SIBYL_PARTICIPANT_H_

namespace sibyl
{

class TimeBounds
{
public:
    constexpr static const int null = -9 * 3600;
    int ref, init, stop, end;
    TimeBounds(int t)                      : ref(t), init(t), stop(t), end(t) {}
    TimeBounds(int r, int i, int s, int e) : ref(r), init(i), stop(s), end(e) {}
};

class Participant
{
public:
    TimeBounds timeBounds;
    void Initialize(TimeBounds timeBounds_) { timeBounds = timeBounds_; InitializeMembers(); }
    
    Participant() : timeBounds(TimeBounds::null) {}
protected:
    virtual void InitializeMembers() = 0;
};

}

#endif /* SIBYL_PARTICIPANT_H_ */