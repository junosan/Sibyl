
#include <fstream>

#include "Simulation_data.h"

namespace sibyl
{

    /* ========================================== */
    /*                  KOSPISim                  */
    /* ========================================== */

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
