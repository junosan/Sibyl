/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_RESHAPER_V1_H_
#define SIBYL_RESHAPER_V1_H_

#include "../Reshaper.h"

#include <array>
#include <vector>
#include <map>

namespace sibyl
{

class Reshaper_v1 : public Reshaper
{
public:
    Reshaper_v1(unsigned long maxGTck_, // this will be ignored and overwritten
                TradeDataSet *pTradeDataSet_,
                std::vector<std::string> *pFileList_,
                const unsigned long (TradeDataSet::* ReadRawFile_)(std::vector<FLOAT>&, CSTR&));

    void ReadConfig(CSTR &filename) override;

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2VecIn(FLOAT *vec, const ItemState &state) override;
    
    /*   ref   -> fractal */
    void Reward2VecOut(FLOAT *vec, const Reward &reward, CSTR &code) override;
    
    /* fractal ->  sibyl  */
    void VecOut2Reward(Reward &reward, const FLOAT *vec, CSTR &code) override;
    
private:
    double b_th, s_th;

    struct ItemMem {
        FLOAT initPr;
        std::array<PQ, idx::szTb> lastTb;
        std::vector<double> idleG;
        std::size_t cursor; // points at idx for current time in idleG
        ItemMem() : initPr(0.0f), lastTb{}, cursor(0) {}
    };
    std::map<STR, ItemMem> items;
};

}

#endif /* SIBYL_RESHAPER_V1_H_ */