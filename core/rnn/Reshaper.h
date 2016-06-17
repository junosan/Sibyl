/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_RESHAPER_H_
#define SIBYL_RESHAPER_H_

#include <map>
#include <cmath>
#include <cassert>
#include <iostream>

#include <Eigen/Eigenvalues>

#include <sibyl/client/ItemState.h>
#include <sibyl/client/Reward.h>
#include "Eigen_helper.h"

class TradeDataSet;

namespace sibyl
{

class ItemMem // storage class for each item
{
public:
    FLOAT initPr;
    
    ItemMem() : initPr(0.0f) {}
};

template <class TItemMem>
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
    std::map<STR, TItemMem> items;
    
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
    void WhitenVector(FLOAT *vec); // to be used by State2VecIn
};

template <class TItemMem>
Reshaper<TItemMem>::Reshaper(unsigned long maxGTck_,
                             TradeDataSet *pTradeDataSet_,
                             std::vector<std::string> *pFileList_,
                             const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, CSTR&, TradeDataSet*))
                             : useWhitening(false)
{
    verify(maxGTck_ >= 0 && pTradeDataSet_ != nullptr && pFileList_ != nullptr && ReadRawFile_ != nullptr);
    maxGTck       = maxGTck_;
    pTradeDataSet = pTradeDataSet_;
    pFileList     = pFileList_;
    ReadRawFile   = ReadRawFile_;
    targetDim     = 2 + 4 * maxGTck;
}

template <class TItemMem>
void Reshaper<TItemMem>::Reward2VecOut(FLOAT *vec, const Reward &reward, CSTR &code)
{
    const auto iItems = items.find(code);
    verify(iItems != std::end(items));
    
    if (vec == nullptr) return; // Derived Reshaper may use this to implement special functions
                                // e.g., rewinding any time-dependent variables
    
    unsigned long idxTarget = 0;
    
    vec[idxTarget++] = ReshapeG_R2V(reward.G0.s);
    vec[idxTarget++] = ReshapeG_R2V(reward.G0.b);
    
    for (std::size_t j = 0; j < maxGTck; j++) vec[idxTarget++] = ReshapeG_R2V(reward.G[j].s );
    for (std::size_t j = 0; j < maxGTck; j++) vec[idxTarget++] = ReshapeG_R2V(reward.G[j].b );
    for (std::size_t j = 0; j < maxGTck; j++) vec[idxTarget++] = ReshapeG_R2V(reward.G[j].cs);
    for (std::size_t j = 0; j < maxGTck; j++) vec[idxTarget++] = ReshapeG_R2V(reward.G[j].cb);
    
    verify(targetDim == idxTarget);
}

template <class TItemMem>
void Reshaper<TItemMem>::VecOut2Reward(Reward &reward, const FLOAT *vec, CSTR &code)
{
    const auto iItems = items.find(code);
    verify(iItems != std::end(items));
    
    unsigned long idxTarget = 0;
    
    reward.G0.s = ReshapeG_V2R(vec[idxTarget++]);
    reward.G0.b = ReshapeG_V2R(vec[idxTarget++]);
    
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].s  = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT)   0.0);
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].b  = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT)   0.0);
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].cs = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT) 100.0);
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].cb = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT) 100.0);
    
    verify(targetDim == idxTarget);
}

template <class TItemMem>
bool Reshaper<TItemMem>::IsWhiteningMatrixValid()
{
    return matMean     .rows() == (typename EMatrix::Index)1        &&
           matMean     .cols() == (typename EMatrix::Index)inputDim &&
           matWhitening.rows() == (typename EMatrix::Index)inputDim &&
           matWhitening.cols() == (typename EMatrix::Index)inputDim &&
           true == Eigen::is_all_finite(matMean)      && // check NaN/inf (custom-made as allFinite() or hasNaN() has some problems)
           true == Eigen::is_all_finite(matWhitening) ;  // check NaN/inf (custom-made as allFinite() or hasNaN() has some problems)
} 

template <class TItemMem>
bool Reshaper<TItemMem>::ReadWhiteningMatrix(CSTR &filename_mean, CSTR &filename_whitening)
{
    bool exists = ( true == Eigen::read_binary(filename_mean     , matMean     ) &&
                    true == Eigen::read_binary(filename_whitening, matWhitening) );                    
    if (true == exists)
    {
        useWhitening = IsWhiteningMatrixValid();
        if (false == useWhitening)
        {
            std::cerr << "Reshaper::ReadWhiteningMatrix: Invalid matrix size or NaN/inf found" << std::endl;
            verify(false);
        }
    } 
    return exists;
}

template <class TItemMem>
void Reshaper<TItemMem>::CalcWhiteningMatrix(CSTR &filename_mean, CSTR &filename_whitening)
{
    // Calculates mean vector and whitening matrix for ZCA whitening
    // See following for explanation:
    // https://en.wikipedia.org/wiki/Whitening_transformation
    // https://en.wikipedia.org/wiki/Sample_mean_and_covariance#Sample_covariance
    
    // NOTE: solver.operatorInverseSqrt() will return NaN matix if non-convergent
    
    unsigned long nSeq = pFileList->size();
    
    // check frame count per file
    verify(nSeq > 0);
    std::vector<FLOAT> vecRaw;
    unsigned long nFrame = ReadRawFile(vecRaw, (*pFileList)[0] + ".raw", pTradeDataSet);
    
    EMatrix M;
    M.resize(nSeq * nFrame, inputDim); // N x K (large)
    
    unsigned long iRow = 0;
    for (unsigned long iSeq = 0; iSeq < nSeq; iSeq++)
    {
        verify(nFrame == ReadRawFile(vecRaw, (*pFileList)[iSeq] + ".raw", pTradeDataSet));
        for (unsigned long iFrame = 0; iFrame < nFrame; iFrame++)
            M.row(iRow++) = Eigen::Map<EMatrix>(vecRaw.data() + iFrame * inputDim, 1, inputDim);
    }
    verify(iRow == nSeq * nFrame && iRow > 1);
    
    matMean = M.colwise().mean();              // 1 x K; implicit type conversion rhs->lhs
    EMatrix M0 = M.rowwise() - matMean.row(0); // N x K; implicit type conversion rhs->lhs
    M.resize(0, 0);                            // N x K -> 0 x 0 (free memory)
    EMatrix Q = (M0.adjoint() * M0) / EScalar(M0.rows() - 1); // K x K
    M0.resize(0, 0);                           // N x K -> 0 x 0 (free memory)
    
    Eigen::SelfAdjointEigenSolver<EMatrix> solver(Q);
    matWhitening = solver.operatorInverseSqrt(); // K x K
    
    // std::cout << "mean vector\n" << matMean << std::endl;
    // std::cout << "whitening matrix\n" << matWhitening << std::endl;
    
    useWhitening = IsWhiteningMatrixValid();
    if (false == useWhitening)
    {
        std::cerr << "Reshaper::CalcWhiteningMatrix: Invalid matrix size or NaN/inf found" << std::endl;
        verify(false);
    }
    
    verify(true == Eigen::write_binary(filename_mean     , matMean     ));
    verify(true == Eigen::write_binary(filename_whitening, matWhitening));
}

template <class TItemMem>
void Reshaper<TItemMem>::DispWhiteningMatrix()
{
    if (true == IsWhiteningMatrixValid())
    {
        std::cout << "mean vector\n"      << matMean.transpose()        << "\n\n"
                  << "whitening matrix\n" << matWhitening               << "\n\n"
                  << "eigenvalues\n"      << matWhitening.eigenvalues() << std::endl;
    }
    else
        std::cout << "Reshaper::DispWhiteningMatrix: No valid matrix initialized" << std::endl;
}

template <class TItemMem>
void Reshaper<TItemMem>::WhitenVector(FLOAT *vec_raw)
{
    if (true == useWhitening)
    {
        Eigen::Map<EMatrix> vec(vec_raw, 1, inputDim); // 1 x K
        vec -= matMean;
        vec *= matWhitening; // out = (in - mean) * W 
    }
}

}

#endif /* SIBYL_RESHAPER_H_ */