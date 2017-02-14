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

#ifndef SIBYL_SERVER_SIMULATION_SIMULATION_TEST_H_
#define SIBYL_SERVER_SIMULATION_SIMULATION_TEST_H_

#include "Simulation_data.h"
#include "../Broker.h"

namespace sibyl
{

// Simulation trying to emulate Kiwoom test server
class Simulation_test : public Broker<OrderSim, ItemSim>
{
public:
    int LoadData(CSTR &config, CSTR &datapath); // non-0 for any error

    // virtuals from Broker
    int   AdvanceTick() override;
    CSTR& BuildMsgOut() override;
    
    void PrintState();
    
    static int ReadTypeExpiry(CSTR &path, CSTR &code); // returns kOptType * expiry (0: non-KOSPI200; skip)
    
    Simulation_test() : nReqThisTick(0) { orderbook.time = -3600 + 600; } // starts at 08:10:10
private:
    void ReadData(int timeTarget); // fill TxtData classes with event info until right before timeTarget
    void SimulateTrades();

    // virtuals from Broker
    int  ExecuteNamedReq(NamedReq<OrderSim, ItemSim> req) override; // non-0 if req count overflow
    void OnExit() override {}
    
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

typedef Simulation_test Simulation;

}

#endif /* SIBYL_SERVER_SIMULATION_SIMULATION_TEST_H_ */