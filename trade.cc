
#include <string>
#include <iostream>

#include <fractal/fractal.h>

#include "rnn/TradeRnn.h"

void printUsage(const char *argv0)
{
    std::cout << "Usage: " << argv0 << " [ -t | -d ] <data path> <workspace path>" << std::endl;
}

int main(int argc, char *argv[])
{
    using namespace fractal;
    
    Engine engine;
    TradeRnn tradeRnn;

    if     (std::string(argv[1]) == "-t" && argc == 4)
    {
        tradeRnn.Configure(engine, TradeRnn::RunType::kTrain, argv[2], argv[3]);
        tradeRnn.Train();
    }
    else if(std::string(argv[1]) == "-d" && argc == 4)
    {
        tradeRnn.Configure(engine, TradeRnn::RunType::kDump , argv[2], argv[3]);
        tradeRnn.Dump();
    }
    else
    {
        printUsage(argv[0]);
        exit(1);
    }

    return 0;
}

