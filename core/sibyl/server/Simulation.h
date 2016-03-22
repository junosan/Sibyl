#ifndef SIBYL_SERVER_SIMULATION_H_
#define SIBYL_SERVER_SIMULATION_H_

#include "Simulation_data.h"
#include "Broker.h"

namespace sibyl
{

class Simulation : public Broker<OrderSim, ItemSim>
{
public:
    int LoadData(CSTR &config, CSTR &datapath); // non-0 for any error

    // virtuals from Broker
    int   AdvanceTick();
    CSTR& BuildMsgOut();
    
    void PrintState();
    
    static int ReadTypeExpiry(CSTR &path, CSTR &code); // returns kOptType * expiry (0: non-KOSPI200; skip)
    
    Simulation() : nReqThisTick(0) {}
private:
    void ReadData(int timeTarget); // fill TxtData classes with event info until right before timeTarget
    void SimulateTrades();

    // virtual from Broker
    int  ExecuteNamedReq(NamedReq<OrderSim, ItemSim> req); // non-0 if req count overflow
    // overrides Broker's implementation
    void InitializeMembers();
    
    // helper functions for LoadData; path should have trailing '/'
    int     DisplayLoadError(CSTR &str); 
    SecType ResolveSecType  (CSTR &path, CSTR &code); // returns kSecNull for failure / already existent

    int nReqThisTick;
    
    // helper functions for OrderSim's B & M
    // add M of all previous orders at p (for new buy/sell)
    INT64 GetM (it_itm_t<ItemSim> iItems, OrdType type, INT p) const;
    // pop B & M for all orders at p, and make the order if B + M becomes negative
    void  PopBM(it_itm_t<ItemSim> iItems, it_ord_t<OrderSim> iOrd, INT q);
    // subtract from M of all subsequent orders at the same price (for cancel/modify)
    void  TrimM(it_itm_t<ItemSim> iItems, it_ord_t<OrderSim> iOrd, INT q);  
};

}

#endif /* SIBYL_SERVER_SIMULATION_H_ */