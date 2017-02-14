/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef REGRESS_RESGATENET_H_
#define REGRESS_RESGATENET_H_

#include "../TradeNet.h"
#include "RegressDataSet.h"
#include "../CustomLayers.h"

#include <string>

namespace fractal
{

template <class TReshaper>
class ResGateNet : public TradeNet<RegressDataSet<TReshaper>>
{
public:
    void Train() override;
private:
    void ConfigureLayers() override;
};

template <class TReshaper>
void ResGateNet<TReshaper>::ConfigureLayers()
{
    auto &rnn = this->rnn;

    unsigned long N = 512;
    unsigned long D = 12;

    InitWeightParamUniform initWeightParam;
    initWeightParam.a = -2e-2;
    initWeightParam.b = 2e-2;
    initWeightParam.isValid = true;

    rnn.AddLayer("BIAS"  , ACT_BIAS  , AGG_DONTCARE, 1);
    rnn.AddLayer("INPUT" , ACT_LINEAR, AGG_DONTCARE, this->inputDim);
    // rnn.AddLayer("RESET" , ACT_LINEAR, AGG_DONTCARE, 1);
    rnn.AddLayer("RESET" , ACT_ONE_MINUS_LINEAR, AGG_DONTCARE, 1);
    rnn.AddLayer("OUTPUT", ACT_LINEAR, AGG_SUM     , this->outputDim);

    auto name = [](const std::string &n, long i, const std::string &m = std::string()) {
        return n + "[" + std::to_string(i) + "]" + (m.empty() ? m : "." + m);
    };

    std::string below = "INPUT";
    auto dim_below = inputDim;

    for (auto i = 0u; i < D; ++i)
    {
        // AddLstmLayer_LearnInit(rnn, name("LSTM", i), "BIAS", 1, N, initWeightParam);
        basicLayers::AddFastLstmLayer(rnn, name("LSTM", i), "BIAS", 1, N, true, initWeightParam);

        rnn.AddConnection(below  , name("LSTM", i, "INPUT"), initWeightParam);
        
        // rnn.AddConnection("RESET", name("LSTM", i, "RESET"), CONN_IDENTITY);
        rnn.AddConnection("RESET", name("LSTM", i, "MEMORY_CELL.DELAYED"), CONN_BROADCAST);
        rnn.AddConnection("RESET", name("LSTM", i, "OUTPUT.DELAYED"), CONN_BROADCAST);

        if (dim_below == N)
        {
            AddResGateLayer(rnn, name("RG", i), "BIAS", N);

            rnn.AddConnection(name("LSTM", i, "OUTPUT"), name("RG", i, "INPUT_R"), CONN_IDENTITY);
            rnn.AddConnection(below                    , name("RG", i, "INPUT_1"), CONN_IDENTITY);

            below = name("RG", i, "OUTPUT");
        }
        else // no resgate when dimension changes
        {
            below = name("LSTM", i, "OUTPUT");
        }

        dim_below = N;
    }

    rnn.AddConnection(below , "OUTPUT", initWeightParam);
    rnn.AddConnection("BIAS", "OUTPUT", initWeightParam);
}

template <class TReshaper>
void ResGateNet<TReshaper>::Train()
{
    auto &rnn             = this->rnn;
    auto &trainDataStream = this->trainDataStream;
    auto &devDataStream   = this->devDataStream;
    verify(this->runType == ResGateNet<TReshaper>::RunType::train);
    
    AutoOptimizer autoOptimizer;

    trainDataStream.SetNumStream(32);
    devDataStream  .SetNumStream(64);

    /* Set ports */
    InputProbe   inputProbe;
    InputProbe   resetProbe;
    RegressProbe outputProbe;

    rnn.LinkProbe(inputProbe , "INPUT");
    rnn.LinkProbe(resetProbe , "RESET");
    rnn.LinkProbe(outputProbe, "OUTPUT");

    PortMapList inputPorts, outputPorts;

    inputPorts.push_back(PortMap(&inputProbe, this->inputChannel));
    inputPorts.push_back(PortMap(&resetProbe, TradeDataSet::CHANNEL_SIG_NEWSEQ));
    outputPorts.push_back(PortMap(&outputProbe, this->outputChannel));

    /* Training */
    {
        autoOptimizer.SetWorkspacePath(this->workspacePath);
        autoOptimizer.SetInitLearningRate(1e-5);
        autoOptimizer.SetMinLearningRate(1e-7);
        autoOptimizer.SetLearningRateDecayRate(0.5);
        autoOptimizer.SetMaxRetryCount(10);
        autoOptimizer.SetMomentum(0.9);
        autoOptimizer.SetWeightNoise(0.0);
        autoOptimizer.SetAdadelta(true);
        //autoOptimizer.SetRmsprop(true);
        autoOptimizer.SetRmsDecayRate(0.99);

        autoOptimizer.Optimize(rnn,
                trainDataStream, devDataStream,
                inputPorts, outputPorts,
                8 * 1024 * 1024, 8 * 1024 * 1024, 256, 128);
    }

    /* Evaluate the best network */

    trainDataStream.SetNumStream(128);
    devDataStream  .SetNumStream(128);

    trainDataStream.Reset();
    devDataStream  .Reset();

    std::cout << "Best network:" << std::endl;

    Evaluator evaluator;

    std::cout << "Train: " << std::endl;
    outputProbe.ResetStatistics();
    evaluator.Evaluate(rnn, trainDataStream, inputPorts, outputPorts, 8 * 1024 * 1024, 32);
    outputProbe.PrintStatistics(std::cout);
    std::cout << std::endl;

    std::cout << "  Dev: " << std::endl;
    outputProbe.ResetStatistics();
    evaluator.Evaluate(rnn, devDataStream, inputPorts, outputPorts, 8 * 1024 * 1024, 32);
    outputProbe.PrintStatistics(std::cout);
    std::cout << std::endl;
}

}

#endif /* REGRESS_RESGATENET_H_ */