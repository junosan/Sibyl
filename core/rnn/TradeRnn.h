/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef TRADERNN_H_
#define TRADERNN_H_

#include <ctime>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>

#include <fractal/fractal.h>

#include "TradeDataSet.h"

namespace fractal
{

class TradeRnn
{
public:
    enum class RunType { null, train, dump, network };
    
    void  Configure(Engine &engine, RunType runType_, const std::string &dataPath_, const std::string &workspacePath_, bool cont = false);
    void  InitUnrollStream(unsigned long nUnroll_, unsigned long nStream_);
    
    void Train();
    void Dump();
    
    /* network */
    TradeDataSet::Reshaper&
           Reshaper    () { return networkData.reshaper; }
    FLOAT* GetInputVec () { verify(nStream > 0); return matInput .GetHostData(); }
    void   RunOneFrame ();
    FLOAT* GetOutputVec() { verify(nStream > 0); return matOutput.GetHostData(); }
    
    TradeRnn() : runType(RunType::null), nStream(0) {}
    ~TradeRnn();
private:
    Rnn rnn;

    RunType runType;
    int frameIdx;

    std::string dataPath;
    std::string workspacePath;
    TradeDataSet trainData, devData, networkData;
    unsigned long inputChannel, outputChannel, inputDim, outputDim;
    DataStream trainDataStream, devDataStream;
    
    unsigned long nUnroll, nStream;
    InputProbe  inputProbe;
    OutputProbe outputProbe;
    Matrix<FLOAT> matInput;
    Matrix<FLOAT> matOutput;
};

}

#endif /* TRADERNN_H_ */