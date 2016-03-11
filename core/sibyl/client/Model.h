#ifndef SIBYL_CLIENT_MODEL_H_
#define SIBYL_CLIENT_MODEL_H_

#include "Portfolio.h"
#include "../Participant.h"

namespace sibyl
{

class Model
{
public:
virtual
    CSTR& BuildMsgOut() = 0; // build ref list based on pPortfolio and other calculated results

    // to be called by Trader
virtual
    void SetStateLogPaths(CSTR &state, CSTR &log) = 0;
    void SetTimeBounds   (TimeBounds timeBounds_) { timeBounds = timeBounds_; }
    void SetPortfolio    (Portfolio *pPortfolio_) { pPortfolio = pPortfolio_; }
    Model() : timeBounds(TimeBounds::null), pPortfolio(nullptr) {} 
protected:
    TimeBounds timeBounds;
    Portfolio *pPortfolio;
};

}

#endif /* SIBYL_CLIENT_MODEL_H_ */