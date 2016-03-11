
#include "Trader.h"

namespace sibyl
{

void Trader::InitializeMembers()
{
    portfolio.SetTimeBounds(timeBounds);
    model    .SetTimeBounds(timeBounds);
    model    .SetPortfolio (&portfolio);
}

void Trader::SetStateLogPaths(CSTR &state, CSTR &log)
{
    portfolio.SetStateLogPaths(state, log);
    model    .SetStateLogPaths(state, log);
}

int Trader::ApplyMsgIn(char *msg)
{
    return portfolio.ApplyMsgIn(msg);
}

CSTR& Trader::BuildMsgOut()
{
    return model.BuildMsgOut();
}

}
