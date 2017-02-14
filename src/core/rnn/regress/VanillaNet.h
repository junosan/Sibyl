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

#ifndef REGRESS_VANILLANET_H_
#define REGRESS_VANILLANET_H_

#include "../TradeNet.h"
#include "RegressDataSet.h"

namespace fractal
{

template <class TReshaper>
class VanillaNet : public TradeNet<RegressDataSet<TReshaper>>
{
public:
    void Train() override;
private:
    void ConfigureLayers() override;
};

template <class TReshaper>
void VanillaNet<TReshaper>::ConfigureLayers()
{
    auto &rnn = this->rnn;

    long N = 1024;

    InitWeightParamUniform initWeightParam;
    initWeightParam.a = -2e-2;
    initWeightParam.b = 2e-2;
    initWeightParam.isValid = true;

    LayerParam dropoutLayerParam;
    dropoutLayerParam.dropoutRate = 0.0;

    rnn.AddLayer("BIAS", ACT_BIAS, AGG_DONTCARE, 1);
    rnn.AddLayer("INPUT", ACT_LINEAR, AGG_DONTCARE, this->inputDim);
    rnn.AddLayer("RESET", ACT_ONE_MINUS_LINEAR, AGG_DONTCARE, 1);
    rnn.AddLayer("OUTPUT", ACT_LINEAR, AGG_SUM, this->outputDim);

    basicLayers::AddFastLstmLayer(rnn, "LSTM[0]", "BIAS", 1, N, true, initWeightParam);
    basicLayers::AddFastLstmLayer(rnn, "LSTM[1]", "BIAS", 1, N, true, initWeightParam);
    basicLayers::AddFastLstmLayer(rnn, "LSTM[2]", "BIAS", 1, N, true, initWeightParam);
    basicLayers::AddFastLstmLayer(rnn, "LSTM[3]", "BIAS", 1, N, true, initWeightParam);
    //    basicLayers::AddFastLstmLayer(rnn, "LSTM[4]", "BIAS", 1, N, true, initWeightParam);

    rnn.AddLayer("LSTM[0].DROPOUT", ACT_DROPOUT, AGG_SUM, N, dropoutLayerParam);
    rnn.AddLayer("LSTM[1].DROPOUT", ACT_DROPOUT, AGG_SUM, N, dropoutLayerParam);
    rnn.AddLayer("LSTM[2].DROPOUT", ACT_DROPOUT, AGG_SUM, N, dropoutLayerParam);
    rnn.AddLayer("LSTM[3].DROPOUT", ACT_DROPOUT, AGG_SUM, N, dropoutLayerParam);
    //    rnn.AddLayer("LSTM[4].DROPOUT", ACT_DROPOUT, AGG_SUM, N, dropoutLayerParam);

    rnn.AddConnection("INPUT", "LSTM[0].INPUT", initWeightParam);
    rnn.AddConnection("RESET", "LSTM[0].MEMORY_CELL.DELAYED", CONN_BROADCAST);
    rnn.AddConnection("RESET", "LSTM[0].OUTPUT.DELAYED", CONN_BROADCAST);
    rnn.AddConnection("LSTM[0].OUTPUT", "LSTM[0].DROPOUT", CONN_IDENTITY);

    rnn.AddConnection("LSTM[0].DROPOUT", "LSTM[1].INPUT", initWeightParam);
    rnn.AddConnection("RESET", "LSTM[1].MEMORY_CELL.DELAYED", CONN_BROADCAST);
    rnn.AddConnection("RESET", "LSTM[1].OUTPUT.DELAYED", CONN_BROADCAST);
    rnn.AddConnection("LSTM[1].OUTPUT", "LSTM[1].DROPOUT", CONN_IDENTITY);

    rnn.AddConnection("LSTM[1].DROPOUT", "LSTM[2].INPUT", initWeightParam);
    rnn.AddConnection("RESET", "LSTM[2].MEMORY_CELL.DELAYED", CONN_BROADCAST);
    rnn.AddConnection("RESET", "LSTM[2].OUTPUT.DELAYED", CONN_BROADCAST);
    rnn.AddConnection("LSTM[2].OUTPUT", "LSTM[2].DROPOUT", CONN_IDENTITY);

    rnn.AddConnection("LSTM[2].DROPOUT", "LSTM[3].INPUT", initWeightParam);
    rnn.AddConnection("RESET", "LSTM[3].MEMORY_CELL.DELAYED", CONN_BROADCAST);
    rnn.AddConnection("RESET", "LSTM[3].OUTPUT.DELAYED", CONN_BROADCAST);
    rnn.AddConnection("LSTM[3].OUTPUT", "LSTM[3].DROPOUT", CONN_IDENTITY);
    /*
       rnn.AddConnection("LSTM[3].DROPOUT", "LSTM[4].INPUT", initWeightParam);
       rnn.AddConnection("RESET", "LSTM[4].MEMORY_CELL.DELAYED", CONN_BROADCAST);
       rnn.AddConnection("RESET", "LSTM[4].OUTPUT.DELAYED", CONN_BROADCAST);
       rnn.AddConnection("LSTM[4].OUTPUT", "LSTM[4].DROPOUT", CONN_IDENTITY);
     */
    rnn.AddConnection("LSTM[3].DROPOUT", "OUTPUT", initWeightParam);
    rnn.AddConnection("BIAS", "OUTPUT", initWeightParam);
}

template <class TReshaper>
void VanillaNet<TReshaper>::Train()
{
    auto &rnn             = this->rnn;
    auto &trainDataStream = this->trainDataStream;
    auto &devDataStream   = this->devDataStream;
    verify(this->runType == VanillaNet<TReshaper>::RunType::train);

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

#endif /* REGRESS_VANILLANET_H_ */