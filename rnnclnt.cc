
#include <ctime>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>

#include <fractal/fractal.h>

#include "TradeDataSet.h"

#include "sibyl/Portfolio.h"
#include "sibyl/NetClient.h"

void printUsage(const char *argv0)
{
    std::cout << "Usage: " << argv0 << " -n <workspace path> <ipaddress> <port> [ -v ]" << std::endl;
}


int main(int argc, char *argv[])
{
    /* ============================================= */
    /*                  Setup sibyl                  */
    /* ============================================= */
    
    using namespace sibyl;
    Portfolio p;
    NetClient nc;
    
    nc.SetPortfolio(&p);
    
    if(std::string(argv[1]) == "-n" && (argc == 5 || argc == 6))
    {
        if ((argc == 6) && (std::string(argv[5]) == "-v"))
            nc.SetVerbose(true);
        else if (argc == 6)
        {
            printUsage(argv[0]);
            exit(1);
        }
    }
    else
    {
        printUsage(argv[0]);
        exit(1);
    }

    const int timeRef  = -3600;        // (08:00:00) last ref price reference
    const int timeInit = kTimeTickSec; // (09:00:10) start input to rnn
    const int timeStop = 21000;        // (14:50:00) stop input to rnn
    const int timeEnd  = 22200;        // (15:10:00) terminate
    p.SetTimeBoundaries(timeRef, timeInit, timeStop, timeEnd);

    const double timeConst = 60.0;
    const double rhoWeight = 30.0;
    const double rhoInit   = 0.001;
    p.SetParams(timeConst, rhoWeight, rhoInit);

    std::string pathState(argv[0]), pathLog(argv[0]);
    auto pos  = pathState.find_last_of('/');
    pathState = pathState.substr(0, pos) + std::string("/state/");
    pathLog   = pathLog  .substr(0, pos) + std::string("/log/");
    p.SetStateLogPaths(pathState, pathLog);
    
    
    /* =============================================== */
    /*                  Setup fractal                  */
    /* =============================================== */
    
    using namespace fractal;
    Rnn rnn;
    Engine engine;
    InitWeightParamUniform initWeightParam;
    unsigned long long randomSeed;

    struct timespec ts;
    unsigned long inputChannel, outputChannel, inputDim, outputDim;

    std::string workspacePath;
    std::string dataPath;

    rnn.SetEngine(&engine);
    rnn.SetComputeLocs({1});


    // initWeightParam.stdev = 1e-2;
    initWeightParam.a = -2e-2;
    initWeightParam.b = 2e-2;
    initWeightParam.isValid = true;
    

    /* Set random seed */
    clock_gettime(CLOCK_MONOTONIC, &ts);
    randomSeed = ts.tv_nsec;
    randomSeed = 0;
    
    TradeDataSet trainData, devData;
    DataStream trainDataStream, devDataStream;

    inputChannel = TradeDataSet::CHANNEL_INPUT;
    outputChannel = TradeDataSet::CHANNEL_TARGET;

    inputDim = devData.GetChannelInfo(inputChannel).frameDim;
    outputDim = devData.GetChannelInfo(outputChannel).frameDim;

    /* Setting random seeds */
    engine.SetRandomSeed(randomSeed);
    trainDataStream.SetRandomSeed(randomSeed);
    devDataStream.SetRandomSeed(randomSeed);


    long N = 2048;

    LayerParam dropoutLayerParam;

    dropoutLayerParam.dropoutRate = 0.0;

    rnn.AddLayer("BIAS", ACT_BIAS, AGG_DONTCARE, 1);
    rnn.AddLayer("INPUT", ACT_LINEAR, AGG_DONTCARE, inputDim);
    rnn.AddLayer("RESET", ACT_ONE_MINUS_LINEAR, AGG_DONTCARE, 1);
    rnn.AddLayer("OUTPUT", ACT_LINEAR, AGG_SUM, outputDim);

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


    workspacePath = argv[2];

    rnn.DeleteLayer("RESET");
    rnn.LoadState(workspacePath + "/net/best/");

    /* ================================================================ */
    /*                  Get data from the RNN output                    */
    /* ================================================================ */

    unsigned long nUnroll = 2, nStream;
    InputProbe inputProbe;
    OutputProbe outputProbe;

    Matrix<FLOAT> matInput;
    Matrix<FLOAT> matOutput;

    /* Set engine */
    matInput.SetEngine(&engine);
    matOutput.SetEngine(&engine);


    /* Link I/O probes */
    rnn.LinkProbe(inputProbe, "INPUT");
    rnn.LinkProbe(outputProbe, "OUTPUT");


    const long interval = kTimeTickSec; // seconds
    const long T = std::ceil((6 * 3600 - 10 * 60)/interval) - 1;
    std::vector<FLOAT> logStartPrice;
    int frameIdx = 0;
    bool initRNN = true;

    
    /* ===================================== */
    /*                  Run                  */
    /* ===================================== */

    /* Connect to server */
    int ret = nc.Connect(std::string(argv[3]), std::string(argv[4]));
    if (ret != 0) exit(1);

    /* Network main loop */
    while (true)
    {
        /* Receive data and fill Portfolio entries */
        ret = nc.RecvNextTick();
        if (ret != 0) break;
        
        if(initRNN == true)
        {
            nStream = p.p.size();

            matInput.Resize(inputDim, nStream);
            matOutput.Resize(outputDim, nStream);

            /* Unroll the network nUnroll times and replicate it nStream times */
            rnn.SetBatchSize(nStream, nUnroll);

            /* Initialize the forward activations */
            rnn.InitForward(0, nUnroll - 1);

            initRNN = false;
        }
        
        // between 09:00:int and 14:50:00
        if ((p.time >= timeInit) && (p.time < timeStop)) 
        {

            /* Generate the input matrix */
            int codeIdx = 0;
            for (auto iP = std::begin(p.p); iP != std::end(p.p); iP++, codeIdx++)
            {
                Item &it = *(iP->second);

                FLOAT *vecIn = matInput.GetHostData() + codeIdx * inputDim;
                FLOAT logCurPrice = (FLOAT) std::log((FLOAT) it.pr) * 100;

                if(frameIdx == 0) {
                    assert((std::size_t)codeIdx == logStartPrice.size());
                    logStartPrice.push_back(logCurPrice);
                }

                // t
                vecIn[0] = (FLOAT) p.time / (interval * T);

                // pr
                vecIn[1] = logCurPrice - logStartPrice[codeIdx];

                // qr
                vecIn[2] = (FLOAT) std::log((FLOAT) 1 + it.qr) / 4;

                // tbpr(1:20)
                for(int i = 0; i < 20; i++)
                {
                    vecIn[3 + i] = (FLOAT) std::log((FLOAT) it.tbr[i].p) * 100 - logStartPrice[codeIdx];
                }

                // tbqr(1:20)
                for(int i = 0; i < 20; i++)
                {
                    vecIn[23 + i] = (FLOAT) std::log((FLOAT) 1 + it.tbr[i].q) / 4;
                }
                
            }

            /* Run RNN */
            {
                int curFrameBufIdx = frameIdx % nUnroll;

                /* Copy the input sequence to the RNN (asynchronous) */
                Matrix<FLOAT> stateSub(inputProbe.GetState(), nStream * curFrameBufIdx, nStream * curFrameBufIdx + nStream - 1);
                Matrix<FLOAT> inputSub(matInput, 0, nStream - 1);

                matInput.HostPush();
                engine.MatCopy(inputSub, stateSub, inputProbe.GetPStream());
                inputProbe.EventRecord();


                /* Forward computation */
                /* Automatically scheduled to be executed after the copy event through the input probe */
                rnn.Forward(curFrameBufIdx, curFrameBufIdx);


                /* Copy the output sequence from the RNN to the GPU memory of matOutput */
                /* Automatically scheduled to be executed after finishing the forward activation */
                Matrix<FLOAT> actSub(outputProbe.GetActivation(), nStream * curFrameBufIdx, nStream * curFrameBufIdx + nStream - 1);
                Matrix<FLOAT> outputSub(matOutput, 0, nStream - 1);

                outputProbe.Wait();
                engine.MatCopy(actSub, outputSub, outputProbe.GetPStream());


                /* Copy the output matrix from the device (GPU) to the host (CPU) */
                matOutput.HostPull(outputProbe.GetPStream());
                outputProbe.EventRecord();

                /* Since the above operations are asynchronous, synchronization is required */
                outputProbe.EventSynchronize();
            }

            /* Get gain values from the output matrix */
            codeIdx = 0;
            for (auto iP = std::begin(p.p); iP != std::end(p.p); iP++, codeIdx++)
            {
                Item &it = *(iP->second);
                
                int i = 0;
                
                FLOAT *vecOut = matOutput.GetHostData() + codeIdx * outputDim;

                it.G0.s = vecOut[i++] / (FLOAT) 100;
                it.G0.b = vecOut[i++] / (FLOAT) 100;

                for(int j = 0; j < 10; j++)                                                                               
                {                                                                                                         
                    if(j < G_MAX_TICK) it.G[j].s = vecOut[i++] / (FLOAT) 100;                                        
                    else               it.G[j].s = 0;                                                                           
                }                                                                                                         
                                                                                                                            
                for(int j = 0; j < 10; j++)                                                                               
                {                                                                                                         
                    if(j < G_MAX_TICK) it.G[j].b = vecOut[i++] / (FLOAT) 100;                                        
                    else               it.G[j].b = 0;                                                                           
                }                                                                                                         
                for(int j = 0; j < 10; j++)                                                                               
                {                                                                                                         
                    if(j < G_MAX_TICK) it.G[j].cs = vecOut[i++] / (FLOAT) 100;                                       
                    else               it.G[j].cs = 100;                                                                        
                }                                                                                                         
                for(int j = 0; j < 10; j++)                                                                               
                {                                                                                                         
                    if(j < G_MAX_TICK) it.G[j].cb = vecOut[i++] / (FLOAT) 100;                                       
                    else               it.G[j].cb = 100;                                                                        
                }                                                                                                         
                verify(i == (int)outputDim);  

                //std::cout << pfGs0[codeIdx] << " " << ppfGs[codeIdx][0] << " " << pfGb0[codeIdx] << " " << ppfGb[codeIdx][0] << std::endl;
            }
            
            frameIdx++;
        }
        
        /* Build orders and send */
        nc.SendResponse();
    }

    rnn.SetEngine(NULL);

    return 0;
}

