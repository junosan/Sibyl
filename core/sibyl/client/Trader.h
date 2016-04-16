#ifndef SIBYL_CLIENT_TRADER_H_
#define SIBYL_CLIENT_TRADER_H_

#include "Portfolio.h"
#include "RewardModel.h"

namespace sibyl
{

class Trader
{
public:
    Portfolio   portfolio;
    RewardModel model;

    void SetStateLogPaths(CSTR &state, CSTR &log) {
        portfolio.SetStateLogPaths(state, log);
        model    .SetStateLogPaths(state, log);
    }
    
    // called by NetClient
    int   ApplyMsgIn (char *msg) { return portfolio.ApplyMsgIn(msg); }
    CSTR& BuildMsgOut()          { return model.BuildMsgOut();       };

    Trader() { model.SetPortfolio(&portfolio); }
};

}

#endif /* SIBYL_CLIENT_TRADER_H_ */