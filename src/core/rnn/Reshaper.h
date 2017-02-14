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

// Defines conversion from sibyl's input state (ItemState) to fractal's input vector
// and conversion between sibyl's output state (Reward) and fractal's target vector
// Implements ZCA whitening for the former case
class Reshaper
{
public:
    // inputDim to be set in derived class's constructor
    Reshaper(unsigned long maxGTck_,
             TradeDataSet *pTradeDataSet_,
             std::vector<std::string> *pFileList_,
             const unsigned long (TradeDataSet::* ReadRawFile_)(std::vector<FLOAT>&, CSTR&));
    
    unsigned long GetMaxGTck  () { return maxGTck;   }
    unsigned long GetInputDim () { return inputDim;  }
    unsigned long GetTargetDim() { return targetDim; }

    bool ReadWhiteningMatrix(CSTR &filename_mean, CSTR &filename_whitening);
    void CalcWhiteningMatrix(CSTR &filename_mean, CSTR &filename_whitening);
    void DispWhiteningMatrix();

    virtual void ReadConfig(CSTR &filename) = 0;

    //   raw   -> fractal (during training)
    //  sibyl  -> fractal (during inference)
    virtual void State2VecIn(FLOAT *vec, const ItemState &state) = 0;
    
    //   ref   -> fractal (during training)
    // (State2VecIn * nFrame) -> (Reward2VecOut * nFrame) (called in batch)
    // NOTE: This will be called with vec = nullptr by TradeDataSet::ReadRefFile
    //       to signal rewind of time-dependent variables (if present) in processing,
    //       allowing time-dependent variables to be implemented whether
    //       input and target processing functions are called in batch or pairs
    virtual void Reward2VecOut(FLOAT *vec, const Reward &reward, CSTR &code);
    
    // fractal ->  sibyl  (during inference)
    // (State2VecIn -> VecOut2Reward) * nFrame (called in pairs every frame)
    virtual void VecOut2Reward(Reward &reward, const FLOAT *vec, CSTR &code);
    
protected:
    virtual FLOAT ReshapePrice(FLOAT p) { return (FLOAT) (std::log(p) * 100.0); }
    // virtual FLOAT ReshapeQuant(INT   q) { return (FLOAT) (std::log((FLOAT) 1 + std::abs(q)) * ((q > 0) - (q < 0)) / 4.0); }
    virtual FLOAT ReshapeQuant(FLOAT q) { return (FLOAT) (std::log(1. + std::abs(q)) * ((q > 0.f) - (q < 0.f)) / 4.0); }
    
    constexpr static const double kR2V = 100.0;
    virtual FLOAT ReshapeG_R2V(FLOAT g) { return (FLOAT) (g * kR2V); }
    virtual FLOAT ReshapeG_V2R(FLOAT g) { return (FLOAT) (g / kR2V); }

    unsigned long maxGTck, inputDim, targetDim;
    void WhitenVector(FLOAT *vec); // to be used by State2VecIn

    bool fullWhitening; // overwrite in a derived class to false if applicable
    
private:
    /* TradeDataSet */
    TradeDataSet *pTradeDataSet;
    std::vector<std::string> *pFileList;
    const unsigned long (TradeDataSet::* ReadRawFile)(std::vector<FLOAT>&, CSTR&);
    
    /* Eigen library */
    typedef float EScalar;
    typedef Eigen::Matrix<EScalar, Eigen::Dynamic, Eigen::Dynamic> EMatrix;
    
    bool useWhitening;
    EMatrix matMean, matWhitening;
    bool IsWhiteningMatrixValid();

};

}

#endif /* SIBYL_RESHAPER_H_ */