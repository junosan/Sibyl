#ifndef SIBYL_SERVER_SIMULATION_DATA_H_
#define SIBYL_SERVER_SIMULATION_DATA_H_

#include <fstream>

#include "OrderBook.h"
#include "../Security.h"
#include "../Security_KOSPI.h"
#include "../Security_ELW.h"
#include "TxtData.h"

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


    /* ========================================== */
    /*                  KOSPISim                  */
    /* ========================================== */

class KOSPISim : public KOSPI<ItemSim>
{
public:
    bool open       (CSTR &path, CSTR &code);
    void AdvanceTime(int timeTarget);
    void SetDelay   (int d);

    KOSPISim() : dataTb(kSecKOSPI) {}
private:
    TxtDataTb dataTb;
};

bool KOSPISim::open(CSTR &path, CSTR &code)
{
    if (true != dataTr.open(path + code + STR(".txt" ))) return false;
    if (true != dataTb.open(path + code + STR("t.txt"))) return false;
    return true;
}

void KOSPISim::AdvanceTime(int timeTarget)
{
    dataTr.InitVecTr();
    dataTr.AdvanceTime(timeTarget);
    
    dataTb.AdvanceTime(timeTarget);
    tbr = dataTb.Tb();
    if (timeTarget > 0) Requantize(dataTr.TrPs1(), dataTr.TrPb1()); // after 09:00:00
    else                Requantize();
}

void KOSPISim::SetDelay(int d)
{
    dataTr.SetDelay(d);
    dataTb.SetDelay(d);
}

    /* ======================================== */
    /*                  ELWSim                  */
    /* ======================================== */

class ELWSim : public ELW<ItemSim>
{
public:
    bool open       (CSTR &path, CSTR &code);
    void AdvanceTime(int timeTarget);
    void SetDelay   (int d);

    ELWSim() : dataTb(kSecELW), dataTh(1 + szTh) {}
    ELWSim(OptType t, INT e) : ELW(t, e), dataTb(kSecELW), dataTh(1 + szTh) {}
private:
    TxtDataTb         dataTb;
    TxtDataVec<FLOAT> dataTh;
    static TxtDataTr dataKOSPI200Tr; // TEMPORARY: using 005930.txt
    static TxtDataTb dataKOSPI200Tb; // TEMPORARY: using 005930t.txt
};
TxtDataTr ELWSim::dataKOSPI200Tr;
TxtDataTb ELWSim::dataKOSPI200Tb(kSecKOSPI);

bool ELWSim::open(CSTR &path, CSTR &code)
{
    if (true != dataTr.open(path + code + STR(".txt" ))) return false;
    if (true != dataTb.open(path + code + STR("t.txt"))) return false;
    if (true != dataTh.open(path + code + STR("g.txt"))) return false;
    if ((dataKOSPI200Tr.is_open() == false) &&
        (true != dataKOSPI200Tr.open(path + STR("005930.txt" )))) return false;
    if ((dataKOSPI200Tb.is_open() == false) &&
        (true != dataKOSPI200Tb.open(path + STR("005930t.txt")))) return false;
    return true;
}

void ELWSim::AdvanceTime(int timeTarget)
{
    dataTr.InitVecTr();
    dataTr.AdvanceTime(timeTarget);
    
    dataTb.AdvanceTime(timeTarget);
    tbr = dataTb.Tb();
    if (timeTarget > 0) Requantize(dataTr.TrPs1(), dataTr.TrPb1()); // after 09:00:00
    else                Requantize();
    
    dataTh.AdvanceTime(timeTarget);
    for (std::size_t idx = 0; idx < szTh; idx++) thr[idx] = dataTh[idx + 1];
    
    dataKOSPI200Tr.InitVecTr();
    dataKOSPI200Tr.AdvanceTime(timeTarget);
    
    dataKOSPI200Tb.AdvanceTime(timeTarget);
    auto temp = dataKOSPI200Tb.Tb();
    if (timeTarget > 0) Requantize(temp, dataKOSPI200Tr.TrPs1(), dataKOSPI200Tr.TrPb1());
    else                Requantize(temp);
    
    kospi200 = TckLo(temp[idxPs1].p) / 5000.0f;
}

void ELWSim::SetDelay(int d)
{
    dataTr.SetDelay(d);
    dataTb.SetDelay(d);
    dataTh.SetDelay(d);
    dataKOSPI200Tr.SetDelay(d);
    dataKOSPI200Tb.SetDelay(d);
}

}

#endif /* SIBYL_SERVER_SIMULATION_DATA_H_ */