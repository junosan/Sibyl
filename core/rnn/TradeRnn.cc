
#include "TradeRnn.h"

namespace fractal
{

TradeRnn::~TradeRnn()
{
    if (runType == RunType::kDump || runType == RunType::kNetwork)
    {
        /* Unlink the probes */
        inputProbe .UnlinkLayer();
        outputProbe.UnlinkLayer();
    }
    if (runType != RunType::kNull) SetEngine(NULL);
}

void TradeRnn::Configure(Engine &engine, RunType runType_, const std::string &dataPath_, const std::string &workspacePath_, bool cont)
{
    assert(runType_ != RunType::kNull);
    
    runType       = runType_;
    dataPath      = dataPath_;
    workspacePath = workspacePath_;

    SetEngine(&engine);
    SetComputeLocs({1});
    
    InitWeightParamUniform initWeightParam;
    initWeightParam.a = -2e-2;
    initWeightParam.b = 2e-2;
    initWeightParam.isValid = true;
    
    /* Set random seed */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned long long randomSeed = ts.tv_nsec;
    
    if (runType == RunType::kTrain || runType == RunType::kDump)
    {
        printf("Random seed: %lld\n\n", randomSeed);
    }
    
    if (runType == RunType::kTrain)
    {
        trainData.ReadFileList(dataPath + "/train.list");
        trainData.ReadData();
        trainDataStream.LinkDataSet(&trainData);
    }

    if (runType == RunType::kTrain || runType == RunType::kDump)
    {
        devData.ReadFileList(dataPath + "/dev.list");
        devData.ReadData();
        devDataStream.LinkDataSet(&devData);
    }

    inputChannel  = TradeDataSet::CHANNEL_INPUT;
    outputChannel = TradeDataSet::CHANNEL_TARGET;

    inputDim  = devData.GetChannelInfo(inputChannel ).frameDim;
    outputDim = devData.GetChannelInfo(outputChannel).frameDim;

    if (runType == RunType::kTrain || runType == RunType::kDump)
    {
        printf("Train: %ld sequences\n", trainData.GetNumSeq());
        printf("  Dev: %ld sequences\n", devData  .GetNumSeq());

        printf("\n");

        printf(" Input dim: %ld\n", inputDim );
        printf("Output dim: %ld\n", outputDim);

        printf("\n");
    }

    /* Setting random seeds */
    engine         .SetRandomSeed(randomSeed);
    trainDataStream.SetRandomSeed(randomSeed);
    devDataStream  .SetRandomSeed(randomSeed);


    long N = 2048;

    LayerParam dropoutLayerParam;

    dropoutLayerParam.dropoutRate = 0.0;

    AddLayer("BIAS", ACT_BIAS, AGG_DONTCARE, 1);
    AddLayer("INPUT", ACT_LINEAR, AGG_DONTCARE, inputDim);
    AddLayer("RESET", ACT_ONE_MINUS_LINEAR, AGG_DONTCARE, 1);
    AddLayer("OUTPUT", ACT_LINEAR, AGG_SUM, outputDim);

    basicLayers::AddFastLstmLayer(*this, "LSTM[0]", "BIAS", 1, N, true, initWeightParam);
    basicLayers::AddFastLstmLayer(*this, "LSTM[1]", "BIAS", 1, N, true, initWeightParam);
    basicLayers::AddFastLstmLayer(*this, "LSTM[2]", "BIAS", 1, N, true, initWeightParam);
    basicLayers::AddFastLstmLayer(*this, "LSTM[3]", "BIAS", 1, N, true, initWeightParam);
    //    basicLayers::AddFastLstmLayer(rnn, "LSTM[4]", "BIAS", 1, N, true, initWeightParam);

    AddLayer("LSTM[0].DROPOUT", ACT_DROPOUT, AGG_SUM, N, dropoutLayerParam);
    AddLayer("LSTM[1].DROPOUT", ACT_DROPOUT, AGG_SUM, N, dropoutLayerParam);
    AddLayer("LSTM[2].DROPOUT", ACT_DROPOUT, AGG_SUM, N, dropoutLayerParam);
    AddLayer("LSTM[3].DROPOUT", ACT_DROPOUT, AGG_SUM, N, dropoutLayerParam);
    //    AddLayer("LSTM[4].DROPOUT", ACT_DROPOUT, AGG_SUM, N, dropoutLayerParam);

    AddConnection("INPUT", "LSTM[0].INPUT", initWeightParam);
    AddConnection("RESET", "LSTM[0].MEMORY_CELL.DELAYED", CONN_BROADCAST);
    AddConnection("RESET", "LSTM[0].OUTPUT.DELAYED", CONN_BROADCAST);
    AddConnection("LSTM[0].OUTPUT", "LSTM[0].DROPOUT", CONN_IDENTITY);

    AddConnection("LSTM[0].DROPOUT", "LSTM[1].INPUT", initWeightParam);
    AddConnection("RESET", "LSTM[1].MEMORY_CELL.DELAYED", CONN_BROADCAST);
    AddConnection("RESET", "LSTM[1].OUTPUT.DELAYED", CONN_BROADCAST);
    AddConnection("LSTM[1].OUTPUT", "LSTM[1].DROPOUT", CONN_IDENTITY);

    AddConnection("LSTM[1].DROPOUT", "LSTM[2].INPUT", initWeightParam);
    AddConnection("RESET", "LSTM[2].MEMORY_CELL.DELAYED", CONN_BROADCAST);
    AddConnection("RESET", "LSTM[2].OUTPUT.DELAYED", CONN_BROADCAST);
    AddConnection("LSTM[2].OUTPUT", "LSTM[2].DROPOUT", CONN_IDENTITY);

    AddConnection("LSTM[2].DROPOUT", "LSTM[3].INPUT", initWeightParam);
    AddConnection("RESET", "LSTM[3].MEMORY_CELL.DELAYED", CONN_BROADCAST);
    AddConnection("RESET", "LSTM[3].OUTPUT.DELAYED", CONN_BROADCAST);
    AddConnection("LSTM[3].OUTPUT", "LSTM[3].DROPOUT", CONN_IDENTITY);
    /*
       AddConnection("LSTM[3].DROPOUT", "LSTM[4].INPUT", initWeightParam);
       AddConnection("RESET", "LSTM[4].MEMORY_CELL.DELAYED", CONN_BROADCAST);
       AddConnection("RESET", "LSTM[4].OUTPUT.DELAYED", CONN_BROADCAST);
       AddConnection("LSTM[4].OUTPUT", "LSTM[4].DROPOUT", CONN_IDENTITY);
     */
    AddConnection("LSTM[3].DROPOUT", "OUTPUT", initWeightParam);
    AddConnection("BIAS", "OUTPUT", initWeightParam);


    if (runType == RunType::kTrain || runType == RunType::kDump)
    {
        PrintNetwork(std::cout);
        printf("Number of weights: %ld\n\n", GetNumWeights());
        
        if (cont == true)
            LoadState(workspacePath + "/net/best/");
    }
    
    if (runType == RunType::kDump || runType == RunType::kNetwork)
    {
        DeleteLayer("RESET");
        LoadState(workspacePath + "/net/best/");
        
        /* Set engine */
        matInput .SetEngine(&engine);
        matOutput.SetEngine(&engine);
        
        /* Link I/O probes */
        LinkProbe(inputProbe , "INPUT");
        LinkProbe(outputProbe, "OUTPUT");
        
        if (runType == RunType::kDump)
            InitUnrollStream(32, 1);
    }
}

void TradeRnn::InitUnrollStream(unsigned long nUnroll_, unsigned long nStream_)
{
    assert(runType == RunType::kDump || runType == RunType::kNetwork);
    
    nUnroll = nUnroll_;
    nStream = nStream_;
    
    if (runType == RunType::kDump)
    {
        matInput .Resize(inputDim , nUnroll * nStream);
        matOutput.Resize(outputDim, nUnroll * nStream);
    }
    
    if (runType == RunType::kNetwork)
    {
        matInput .Resize(inputDim , nStream);
        matOutput.Resize(outputDim, nStream);
        frameIdx = -1; // starts at 0 when RunOneFrame() is called
    }
    
    /* Unroll the network nUnroll times and replicate it nStream times */
    SetBatchSize(nStream, nUnroll);

    /* Initialize the forward activations */
    InitForward(0, nUnroll - 1);
}

void TradeRnn::Train()
{
    assert(runType == RunType::kTrain);
    
    AutoOptimizer autoOptimizer;

    trainDataStream.SetNumStream(30);
    devDataStream  .SetNumStream(64);

    /* Set ports */
    InputProbe   inputProbe;
    InputProbe   resetProbe;
    RegressProbe outputProbe;

    LinkProbe(inputProbe , "INPUT");
    LinkProbe(resetProbe , "RESET");
    LinkProbe(outputProbe, "OUTPUT");

    PortMapList inputPorts, outputPorts;

    inputPorts.push_back(PortMap(&inputProbe, inputChannel));
    inputPorts.push_back(PortMap(&resetProbe, TradeDataSet::CHANNEL_SIG_NEWSEQ));
    outputPorts.push_back(PortMap(&outputProbe, outputChannel));
#if 1
    /* Training */
    {
        autoOptimizer.SetWorkspacePath(workspacePath);
        autoOptimizer.SetInitLearningRate(1e-5);
        autoOptimizer.SetMinLearningRate(1e-7);
        autoOptimizer.SetLearningRateDecayRate(0.5);
        autoOptimizer.SetMaxRetryCount(10);
        autoOptimizer.SetMomentum(0.9);
        autoOptimizer.SetWeightNoise(0.0);
        autoOptimizer.SetAdadelta(true);
        //autoOptimizer.SetRmsprop(true);
        autoOptimizer.SetRmsDecayRate(0.99);

        autoOptimizer.Optimize(*this,
                trainDataStream, devDataStream,
                inputPorts, outputPorts,
                2 * 1024 * 1024, 2 * 1024 * 1024, 128, 64);
    }
#endif

    /* Evaluate the best network */

    trainDataStream.SetNumStream(128);
    devDataStream  .SetNumStream(128);

    trainDataStream.Reset();
    devDataStream  .Reset();

    std::cout << "Best network:" << std::endl;

    Evaluator evaluator;

    std::cout << "Train: " << std::endl;
    outputProbe.ResetStatistics();
    evaluator.Evaluate(*this, trainDataStream, inputPorts, outputPorts, 4 * 1024 * 1024, 32);
    outputProbe.PrintStatistics(std::cout);
    std::cout << std::endl;

    std::cout << "  Dev: " << std::endl;
    outputProbe.ResetStatistics();
    evaluator.Evaluate(*this, devDataStream, inputPorts, outputPorts, 4 * 1024 * 1024, 32);
    outputProbe.PrintStatistics(std::cout);
    std::cout << std::endl;
}

void TradeRnn::Dump()
{
    assert(runType == RunType::kDump);

    unsigned long seqIdx = 83;
    unsigned long nFrame = devData.GetNumFrame(seqIdx);

    FILE *file = fopen("dump.txt", "w");

    std::cout << "Dumping RNN output for " << devData.GetFilename(seqIdx) << std::endl;

    for(unsigned long j = 0; j < nFrame; j += nUnroll)
    {
        unsigned long curNumFrame = std::min(nUnroll, nFrame - j);

        /* Copy the input sequence to the RNN (asynchronous) */
        Matrix<FLOAT> stateSub(inputProbe.GetState(), 0, nStream * curNumFrame - 1);
        Matrix<FLOAT> inputSub(matInput, 0, nStream * curNumFrame - 1);

        for(unsigned long i = 0; i < curNumFrame; i++)
        {
            devData.GetFrameData(seqIdx, inputChannel, j + i, inputSub.GetHostData() + i * inputDim * nStream);
        }

        matInput.HostPush();
        GetEngine()->MatCopy(inputSub, stateSub, inputProbe.GetPStream());
        inputProbe.EventRecord();


        /* Forward computation */
        /* Automatically scheduled to be executed after the copy event through the input probe */
        Forward(0, curNumFrame - 1);


        /* Copy the output sequence from the RNN to the GPU memory of matOutput */
        /* Automatically scheduled to be executed after finishing the forward activation */
        Matrix<FLOAT> actSub(outputProbe.GetActivation(), 0, nStream * curNumFrame - 1);
        Matrix<FLOAT> outputSub(matOutput, 0, nStream * curNumFrame - 1);

        outputProbe.Wait();
        GetEngine()->MatCopy(actSub, outputSub, outputProbe.GetPStream());


        /* Copy the output matrix from the device (GPU) to the host (CPU) */
        matOutput.HostPull(outputProbe.GetPStream());
        outputProbe.EventRecord();

        /* Since the above operations are asynchronous, synchronization is required */
        outputProbe.EventSynchronize();


        for(unsigned long i = 0; i < curNumFrame; i++)
        {
            for(unsigned long k = 0; k < outputDim; k++)
            {
                FLOAT val = outputSub.GetHostData()[i * outputDim * nStream + k];
                fprintf(file, "%g\t", val);
            }
            fprintf(file, "\n");
        }
    }

    fclose(file);
}

void TradeRnn::RunOneFrame()
{
    assert(runType == RunType::kNetwork);
    
    int curFrameBufIdx = (++frameIdx) % nUnroll;

    /* Copy the input sequence to the RNN (asynchronous) */
    Matrix<FLOAT> stateSub(inputProbe.GetState(), nStream * curFrameBufIdx, nStream * curFrameBufIdx + nStream - 1);
    Matrix<FLOAT> inputSub(matInput, 0, nStream - 1);

    matInput.HostPush();
    GetEngine()->MatCopy(inputSub, stateSub, inputProbe.GetPStream());
    inputProbe.EventRecord();


    /* Forward computation */
    /* Automatically scheduled to be executed after the copy event through the input probe */
    Forward(curFrameBufIdx, curFrameBufIdx);


    /* Copy the output sequence from the RNN to the GPU memory of matOutput */
    /* Automatically scheduled to be executed after finishing the forward activation */
    Matrix<FLOAT> actSub(outputProbe.GetActivation(), nStream * curFrameBufIdx, nStream * curFrameBufIdx + nStream - 1);
    Matrix<FLOAT> outputSub(matOutput, 0, nStream - 1);

    outputProbe.Wait();
    GetEngine()->MatCopy(actSub, outputSub, outputProbe.GetPStream());


    /* Copy the output matrix from the device (GPU) to the host (CPU) */
    matOutput.HostPull(outputProbe.GetPStream());
    outputProbe.EventRecord();

    /* Since the above operations are asynchronous, synchronization is required */
    outputProbe.EventSynchronize();
}

}
