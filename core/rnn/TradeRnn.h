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

class TradeRnn : public Rnn
{
public:
    enum class RunType { kNull, kTrain, kDump, kNetwork };
    
    void Configure(Engine &engine, RunType runType_, const std::string &dataPath_, const std::string &workspacePath_, bool cont = false);
    void InitUnrollStream(unsigned long nUnroll_, unsigned long nStream_);
    
    /*  kTrain  */
    void Train();
    
    /*  kDump  */
    void Dump();
    
    /* kNetwork */
    FLOAT* GetInputVec () { assert(nStream > 0); return matInput .GetHostData(); }
    void   RunOneFrame ();
    FLOAT* GetOutputVec() { assert(nStream > 0); return matOutput.GetHostData(); }
    
    TradeRnn() : runType(RunType::kNull), nStream(0) {}
    ~TradeRnn();
protected:
    RunType runType;
    int frameIdx;

    std::string dataPath;
    std::string workspacePath;
    TradeDataSet trainData, devData;
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