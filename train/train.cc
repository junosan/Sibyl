/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include <string>
#include <iostream>

#include <fractal/fractal.h>

#include <rnn/TradeRnn.h>

int main(int argc, char *argv[])
{
    if ( (argc != 4)  ||
         ( std::string(argv[1]) != "-t"  && // train
           std::string(argv[1]) != "-tc" && // train (continue from /net/best)
           std::string(argv[1]) != "-d"     // dump
         ) )
    {
        std::cerr << "USAGE: train [ -t | -tc | -d ] <data path> <workspace path>" << std::endl;
        exit(1);
    }
    
    using namespace fractal;
    
    Engine engine;
    TradeRnn tradeRnn;

    if     (std::string(argv[1]) == "-t" || std::string(argv[1]) == "-tc")
    {
        tradeRnn.Configure(engine, TradeRnn::RunType::kTrain, argv[2], argv[3], std::string(argv[1]) == "-tc");
        tradeRnn.Train();
    }
    else if(std::string(argv[1]) == "-d")
    {
        tradeRnn.Configure(engine, TradeRnn::RunType::kDump , argv[2], argv[3]);
        tradeRnn.Dump();
    }

    return 0;
}

