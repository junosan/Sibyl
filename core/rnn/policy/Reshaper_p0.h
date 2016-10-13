/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_RESHAPER_P0_H_
#define SIBYL_RESHAPER_P0_H_

#include "../Reshaper.h"

#include <array>
#include <vector>

namespace sibyl
{

class ItemMem_p0 : public ItemMem
{
public:
    std::array<PQ, idx::szTb> lastTb;
    ItemMem_p0() : ItemMem(), lastTb{} {}
};

class Reshaper_p0 : public Reshaper<ItemMem_p0>
{
public:
    Reshaper_p0(unsigned long maxGTck_, // this will be ignored and overwritten
                TradeDataSet *pTradeDataSet_,
                std::vector<std::string> *pFileList_,
                const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, CSTR&, TradeDataSet*));

    void ReadConfig(CSTR &filename) override;

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2VecIn(FLOAT *vec, const ItemState &state) override;
    
    /*   ref   -> fractal */
    void Reward2VecOut(FLOAT *vec, const Reward &reward, CSTR &code) override;
    
    /* fractal ->  sibyl  */
    void VecOut2Reward(Reward &reward, const FLOAT *vec, CSTR &code) override;
    
    struct vecout_idx {
        static constexpr std::size_t idle = 0;
        static constexpr std::size_t sell = 1;
        static constexpr std::size_t buy  = 2;
    };

private:
    double b_th, s_th;
};

}

#endif /* SIBYL_RESHAPER_P0_H_ */