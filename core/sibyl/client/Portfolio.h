#ifndef SIBYL_CLIENT_PORTFOLIO_H_
#define SIBYL_CLIENT_PORTFOLIO_H_

#include <vector>
#include <fstream>

#include "../Security.h"
#include "../Security_KOSPI.h"
#include "../Security_ELW.h"
#include "../Security_ETF.h"
#include "../Catalog.h"
#include "ItemState.h"

namespace sibyl
{

class Order : public PQ
{
public:
    OrdType type;
    Order() : type(kOrdNull) {}
};

class Item : public Security<Order> // abstract class; derive as stock, ELW, ETF, option, etc.
{
public:
    virtual ~Item() {}
};

class Portfolio : public Catalog<Item>
{
public:
    // to be used by rnn client
    const std::vector<ItemState>& GetStateVec();
    
    // to be called by Trader
    void SetStateLogPaths(CSTR &state, CSTR &log);
    int  ApplyMsgIn      (char *msg); // this destroys msg during parsing; returns non-0 to signal termination
private:
    std::vector<ItemState> vecState;
    
    STR pathState;
    STR pathLog;
    
    void WriteState(); // writes only if state path was set
    
    std::ofstream logMsgIn;  // full state msg
    std::ofstream logVecOut; // t, pr, qr, tbr
    
    char bufLine[1 << 10];
};

}

#endif  /* SIBYL_CLIENT_PORTFOLIO_H_ */