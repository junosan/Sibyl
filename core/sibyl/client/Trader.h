#ifndef SIBYL_CLIENT_TRADER_H_
#define SIBYL_CLIENT_TRADER_H_

#include "../Participant.h"
#include "Portfolio.h"
#include "RewardModel.h"

namespace sibyl
{

class Trader : public Participant
{
public:
    Portfolio   portfolio;
    RewardModel model;

    void SetStateLogPaths(CSTR &state, CSTR &log); 
    
    // called by NetClient
    int   ApplyMsgIn (char *msg); // this destroys msg during parsing; returns non-0 to signal termination
    CSTR& BuildMsgOut();
private:
    // virtual from Participant
    void InitializeMembers();
};

}

#endif /* SIBYL_CLIENT_TRADER_H_ */