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

#ifndef TRADENET_H_
#define TRADENET_H_

#include <fractal/fractal.h>

#include <ctime>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>

namespace fractal
{

// Encapsulates code blocks for fractal::Rnn training or inference
// Base class for specific neural net architectures (e.g., VanillaNet)
template <class TDataSet>
class TradeNet
{
public:
    enum class RunType { null, train, network };
    
    // To train: Configure -> Train
    // To run  : Configure -> InitUnrollStream ->
    //           { GetInputVec  -> (fill input data) -> 
    //             RunOneFrame  ->
    //             GetOutputVec -> (use output data) } x N
    void Configure(Engine &engine, RunType runType_,
                   const std::string &dataPath_, const std::string &workspacePath_, bool cont = false);
    
    virtual void Train() = 0;
    
    /* Below are for RunType::network */
    sibyl::Reshaper& Reshaper() { return networkData.Reshaper(); }

    void InitUnrollStream(unsigned long nUnroll_, unsigned long nStream_);
    void RunOneFrame();

    FLOAT* GetInputVec () { verify(nStream > 0); return matInput .GetHostData(); }
    FLOAT* GetOutputVec() { verify(nStream > 0); return matOutput.GetHostData(); }
    
    TradeNet() : runType(RunType::null), frameIdx(0), nUnroll(0), nStream(0) {}
    ~TradeNet();

protected:
    virtual void ConfigureLayers() = 0;

    RunType runType;
    std::string workspacePath;

    Rnn rnn;
    unsigned long inputChannel, outputChannel;
    unsigned long inputDim, outputDim;
    DataStream trainDataStream, devDataStream;
    
private:
    std::string dataPath;
    TDataSet trainData, devData, networkData;   
    
    int frameIdx;
    unsigned long nUnroll, nStream;

    InputProbe  inputProbe;
    OutputProbe outputProbe;

    Matrix<FLOAT> matInput;
    Matrix<FLOAT> matOutput;
};

template <class TDataSet>
TradeNet<TDataSet>::~TradeNet()
{
    if (runType == RunType::network)
    {
        inputProbe .UnlinkLayer();
        outputProbe.UnlinkLayer();
    }

    if (runType != RunType::null)
        rnn.SetEngine(NULL);
}

template <class TDataSet>
void TradeNet<TDataSet>::Configure(Engine &engine, RunType runType_,
                         const std::string &dataPath_, const std::string &workspacePath_, bool cont)
{
    verify(runType_ != RunType::null);
    
    runType       = runType_;
    dataPath      = dataPath_;
    workspacePath = workspacePath_;

    rnn.SetEngine(&engine);
    rnn.SetComputeLocs({1});
    
    /* Configure channels */
    inputChannel  = TDataSet::CHANNEL_INPUT;
    outputChannel = TDataSet::CHANNEL_TARGET;

    inputDim  = devData.GetChannelInfo(inputChannel ).frameDim;
    outputDim = devData.GetChannelInfo(outputChannel).frameDim;

    /* Set random seed */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned long long randomSeed = ts.tv_nsec;
    
    engine         .SetRandomSeed(randomSeed);
    trainDataStream.SetRandomSeed(randomSeed);
    devDataStream  .SetRandomSeed(randomSeed);

    if (runType == RunType::train)
    {
        printf("Random seed: %lld\n\n", randomSeed);
        
        /* Prepare train data */
        trainData.ReadFileList(dataPath + "/train.list");

        if (false == cont &&
            false == trainData.Reshaper().ReadWhiteningMatrix(workspacePath + "/mean.matrix", workspacePath + "/whitening.matrix"))
        {
            verify(workspacePath != "");
            verify(system(std::string("mkdir -p " + workspacePath).c_str()) == 0);
            trainData.Reshaper().CalcWhiteningMatrix(workspacePath + "/mean.matrix", workspacePath + "/whitening.matrix");
        }
        verify(true == trainData.Reshaper().ReadWhiteningMatrix(workspacePath + "/mean.matrix", workspacePath + "/whitening.matrix"));
        
        trainData.ReadData();
        trainDataStream.LinkDataSet(&trainData);

        /* Prepare dev data */
        devData.ReadFileList(dataPath + "/dev.list");

        verify(true == devData.Reshaper().ReadWhiteningMatrix(workspacePath + "/mean.matrix", workspacePath + "/whitening.matrix"));

        devData.ReadData();
        devDataStream.LinkDataSet(&devData);

        printf("Train: %ld sequences\n", trainData.GetNumSeq());
        printf("  Dev: %ld sequences\n", devData  .GetNumSeq());

        printf("\n");

        printf(" Input dim: %ld\n", inputDim );
        printf("Output dim: %ld\n", outputDim);

        printf("\n");
    }

    ConfigureLayers();

    if (runType == RunType::train)
    {
        rnn.PrintNetwork(std::cout);
        printf("Number of weights: %ld\n\n", rnn.GetNumWeights());
        
        if (cont == true)
            rnn.LoadState(workspacePath + "/net/best/");
    }
    
    if (runType == RunType::network)
    {
        rnn.DeleteLayer("RESET");
        rnn.LoadState(workspacePath + "/net/best/");
        
        /* Set engine */
        matInput .SetEngine(&engine);
        matOutput.SetEngine(&engine);
        
        /* Link I/O probes */
        rnn.LinkProbe(inputProbe , "INPUT");
        rnn.LinkProbe(outputProbe, "OUTPUT");
        
        networkData.Reshaper().ReadWhiteningMatrix(workspacePath + "/mean.matrix", workspacePath + "/whitening.matrix");
    }
}

template <class TDataSet>
void TradeNet<TDataSet>::InitUnrollStream(unsigned long nUnroll_, unsigned long nStream_)
{
    verify(runType == RunType::network && nUnroll_ > 0 && nStream_ > 0);
    
    nUnroll = nUnroll_;
    nStream = nStream_;

    matInput .Resize(inputDim , nStream);
    matOutput.Resize(outputDim, nStream);
    frameIdx = -1; // starts at 0 when RunOneFrame() is called
    
    /* Unroll the network nUnroll times and replicate it nStream times */
    rnn.SetBatchSize(nStream, nUnroll);

    /* Initialize the forward activations */
    rnn.InitForward(0, nUnroll - 1);
}

template <class TDataSet>
void TradeNet<TDataSet>::RunOneFrame()
{
    verify(runType == RunType::network && nUnroll > 0 && nStream > 0);
    
    int curFrameBufIdx = (++frameIdx) % nUnroll;

    /* Copy the input sequence to the RNN (asynchronous) */
    Matrix<FLOAT> stateSub(inputProbe.GetState(), nStream * curFrameBufIdx, nStream * curFrameBufIdx + nStream - 1);
    Matrix<FLOAT> inputSub(matInput, 0, nStream - 1);

    matInput.HostPush();
    rnn.GetEngine()->MatCopy(inputSub, stateSub, inputProbe.GetPStream());
    inputProbe.EventRecord();


    /* Forward computation */
    /* Automatically scheduled to be executed after the copy event through the input probe */
    rnn.Forward(curFrameBufIdx, curFrameBufIdx);


    /* Copy the output sequence from the RNN to the GPU memory of matOutput */
    /* Automatically scheduled to be executed after finishing the forward activation */
    Matrix<FLOAT> actSub(outputProbe.GetActivation(), nStream * curFrameBufIdx, nStream * curFrameBufIdx + nStream - 1);
    Matrix<FLOAT> outputSub(matOutput, 0, nStream - 1);

    outputProbe.Wait();
    rnn.GetEngine()->MatCopy(actSub, outputSub, outputProbe.GetPStream());


    /* Copy the output matrix from the device (GPU) to the host (CPU) */
    matOutput.HostPull(outputProbe.GetPStream());
    outputProbe.EventRecord();

    /* Since the above operations are asynchronous, synchronization is required */
    outputProbe.EventSynchronize();
}

}

#endif /* TRADENET_H_ */