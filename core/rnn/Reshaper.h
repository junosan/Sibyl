/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_RESHAPER_H_
#define SIBYL_RESHAPER_H_

#include <sibyl/client/ItemState.h>
#include <sibyl/client/Reward.h>
#include <sibyl/util/Eigen_helper.h>

#include <Eigen/Eigenvalues>

#include <cmath>
#include <cassert>
#include <iostream>

class TradeDataSet;

namespace sibyl
{

class Reshaper
{
public:
    // inputDim to be set in derived class's constructor
    Reshaper(unsigned long maxGTck_,
             TradeDataSet *pTradeDataSet_,
             std::vector<std::string> *pFileList_,
             const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, CSTR&, TradeDataSet*));
    
    unsigned long GetMaxGTck  () { return maxGTck;   }
    unsigned long GetInputDim () { return inputDim;  }
    unsigned long GetTargetDim() { return targetDim; }

    bool ReadWhiteningMatrix(CSTR &filename_mean, CSTR &filename_whitening);
    void CalcWhiteningMatrix(CSTR &filename_mean, CSTR &filename_whitening);
    void DispWhiteningMatrix();

    virtual void ReadConfig(CSTR &filename) = 0;

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    virtual void State2VecIn(FLOAT *vec, const ItemState &state) = 0;
    
    /*   ref   -> fractal */ /* (State2VecIn)xN, (Reward2VecOut)xN (called in batch) */
    virtual void Reward2VecOut(FLOAT *vec, const Reward &reward, CSTR &code);
    
    /* fractal ->  sibyl  */ /* (State2VecIn   ,  VecOut2Reward)xN (called in pairs) */
    virtual void VecOut2Reward(Reward &reward, const FLOAT *vec, CSTR &code);
    
protected:
    virtual FLOAT ReshapePrice(FLOAT p) { return (FLOAT) (std::log((FLOAT) p) * 100.0); }
    virtual FLOAT ReshapeQuant(INT   q) { return (FLOAT) (std::log((FLOAT) 1 + std::abs(q)) * ((q > 0) - (q < 0)) / 4.0); }
    
    constexpr static const double kR2V = 100.0;
    virtual FLOAT ReshapeG_R2V(FLOAT g) { return (FLOAT) (g * kR2V); }
    virtual FLOAT ReshapeG_V2R(FLOAT g) { return (FLOAT) (g / kR2V); }

    unsigned long maxGTck, inputDim, targetDim;
    void WhitenVector(FLOAT *vec); // to be used by State2VecIn
    
private:
    /* TradeDataSet */
    TradeDataSet *pTradeDataSet;
    std::vector<std::string> *pFileList;
    const unsigned long (*ReadRawFile)(std::vector<FLOAT>&, CSTR&, TradeDataSet*);
    
    /* Eigen library */
    typedef float EScalar;
    typedef Eigen::Matrix<EScalar, Eigen::Dynamic, Eigen::Dynamic> EMatrix;
    
    bool useWhitening;
    EMatrix matMean, matWhitening;
    bool IsWhiteningMatrixValid();

};

}

#endif /* SIBYL_RESHAPER_H_ */