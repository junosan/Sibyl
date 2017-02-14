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

#include "Reshaper.h"

namespace sibyl
{

Reshaper::Reshaper(unsigned long maxGTck_,
                             TradeDataSet *pTradeDataSet_,
                             std::vector<std::string> *pFileList_,
                             const unsigned long (TradeDataSet::* ReadRawFile_)(std::vector<FLOAT>&, CSTR&))
                             : fullWhitening(true), useWhitening(false)
{
    verify(maxGTck_ >= 0 && pTradeDataSet_ != nullptr && pFileList_ != nullptr && ReadRawFile_ != nullptr);
    maxGTck       = maxGTck_;
    pTradeDataSet = pTradeDataSet_;
    pFileList     = pFileList_;
    ReadRawFile   = ReadRawFile_;
    targetDim     = 2 + 4 * maxGTck;
}

void Reshaper::Reward2VecOut(FLOAT *vec, const Reward &reward, CSTR &code)
{
    // const auto iItems = items.find(code);
    // verify(iItems != std::end(items));
    
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

void Reshaper::VecOut2Reward(Reward &reward, const FLOAT *vec, CSTR &code)
{
    // const auto iItems = items.find(code);
    // verify(iItems != std::end(items));
    
    unsigned long idxTarget = 0;
    
    reward.G0.s = ReshapeG_V2R(vec[idxTarget++]);
    reward.G0.b = ReshapeG_V2R(vec[idxTarget++]);
    
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].s  = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT)   0.0);
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].b  = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT)   0.0);
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].cs = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT) 100.0);
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].cb = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT) 100.0);
    
    verify(targetDim == idxTarget);
}

bool Reshaper::IsWhiteningMatrixValid()
{
    return matMean     .rows() == (typename EMatrix::Index)1        &&
           matMean     .cols() == (typename EMatrix::Index)inputDim &&
           matWhitening.rows() == (typename EMatrix::Index)inputDim &&
           matWhitening.cols() == (typename EMatrix::Index)inputDim &&
           true == Eigen::is_all_finite(matMean)      && // check NaN/inf (custom-made as allFinite() or hasNaN() has some problems)
           true == Eigen::is_all_finite(matWhitening) ;  // check NaN/inf (custom-made as allFinite() or hasNaN() has some problems)
} 

bool Reshaper::ReadWhiteningMatrix(CSTR &filename_mean, CSTR &filename_whitening)
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

void Reshaper::CalcWhiteningMatrix(CSTR &filename_mean, CSTR &filename_whitening)
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
    unsigned long nFrame = (pTradeDataSet->*ReadRawFile)(vecRaw, (*pFileList)[0] + ".raw");
    
    EMatrix M;
    M.resize(nSeq * nFrame, inputDim); // N x K (large)
    
    unsigned long iRow = 0;
    for (unsigned long iSeq = 0; iSeq < nSeq; iSeq++)
    {
        verify(nFrame == (pTradeDataSet->*ReadRawFile)(vecRaw, (*pFileList)[iSeq] + ".raw"));
        for (unsigned long iFrame = 0; iFrame < nFrame; iFrame++)
            M.row(iRow++) = Eigen::Map<EMatrix>(vecRaw.data() + iFrame * inputDim, 1, inputDim);
    }
    verify(iRow == nSeq * nFrame && iRow > 1);
    
    matMean = M.colwise().mean();              // 1 x K; implicit type conversion rhs->lhs
    EMatrix M0 = M.rowwise() - matMean.row(0); // N x K; implicit type conversion rhs->lhs
    M.resize(0, 0);                            // N x K -> 0 x 0 (free memory)
    EMatrix Q = (M0.adjoint() * M0) / EScalar(M0.rows() - 1); // K x K
    M0.resize(0, 0);                           // N x K -> 0 x 0 (free memory)
    
    // Use Q for full ZCA whitening
    // Use Q.diagonal().asDiagonal() for mean/stdev compensation only
    EMatrix R = (fullWhitening == true ? Q : Q.diagonal().asDiagonal());

    Eigen::SelfAdjointEigenSolver<EMatrix> solver(R);
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

void Reshaper::DispWhiteningMatrix()
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

void Reshaper::WhitenVector(FLOAT *vec_raw)
{
    if (true == useWhitening)
    {
        Eigen::Map<EMatrix> vec(vec_raw, 1, inputDim); // 1 x K
        vec -= matMean;
        vec *= matWhitening; // out = (in - mean) * W 
    }
}

}