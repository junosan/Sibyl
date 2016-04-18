#ifndef SIBYL_SERVER_SIMULATION_SIMULATION_DATA_H_
#define SIBYL_SERVER_SIMULATION_SIMULATION_DATA_H_

#include "../OrderBook.h"
#include "../../Security.h"
#include "TxtData.h"
#include "../NetServer.h"

namespace sibyl
{

class OrderSim : public Order
{
public:
    INT64 B; // # of orders queued by others before this order
    INT64 M; // # of orders queued by   me   before this order
    OrderSim() : B(0), M(0) {}
};

class ItemSim : public Item<OrderSim>
{
public:
    virtual bool open       (CSTR &path, CSTR &code) = 0;
    virtual void AdvanceTime(int timeTarget)         = 0;
    virtual void SetDelay   (int d)                  = 0;
    TxtDataTr&   TrData     () { return dataTr; }   // use this to InitSum() or const access vecTr

    INT depS0, depB0; // "depletion"
 
    ItemSim() : depS0(0), depB0(0) {}
protected:
    TxtDataTr dataTr;
};

typedef NetServer<OrderSim, ItemSim> SimulationServer;


    /* ========================================== */
    /*                  KOSPISim                  */
    /* ========================================== */

class KOSPISim : public KOSPI<ItemSim>
{
public:
    bool open       (CSTR &path, CSTR &code);
    void AdvanceTime(int timeTarget);
    void SetDelay   (int d);

    KOSPISim() : dataTb(SecType::KOSPI) {}
private:
    TxtDataTb dataTb;
};


    /* ======================================== */
    /*                  ELWSim                  */
    /* ======================================== */

class ELWSim : public ELW<ItemSim>
{
public:
    bool open       (CSTR &path, CSTR &code);
    void AdvanceTime(int timeTarget);
    void SetDelay   (int d);

    ELWSim()                 :            dataTb(SecType::ELW), dataTh(1 + szTh) {}
    ELWSim(OptType t, INT e) : ELW(t, e), dataTb(SecType::ELW), dataTh(1 + szTh) {}
private:
    TxtDataTb         dataTb;
    TxtDataVec<FLOAT> dataTh;
    static TxtDataTr dataKOSPI200Tr; // TEMPORARY: using 005930.txt
    static TxtDataTb dataKOSPI200Tb; // TEMPORARY: using 005930t.txt
};


    /* ======================================== */
    /*                  ETFSim                  */
    /* ======================================== */

class ETFSim : public ETF<ItemSim>
{
public:
    bool open       (CSTR &path, CSTR &code);
    void AdvanceTime(int timeTarget);
    void SetDelay   (int d);

    ETFSim() : dataTb(SecType::ETF), dataNAV(2) {}
private:
    TxtDataTb          dataTb;
    TxtDataVec<double> dataNAV;
};

}

#endif /* SIBYL_SERVER_SIMULATION_SIMULATION_DATA_H_ */