/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include <string>
#include <iostream>

#include <fractal/fractal.h>

#include <rnn/value/ValueNet.h>
typedef fractal::ValueNet Net;

int main(int argc, char *argv[])
{
    if ( (argc != 4)  ||
         ( std::string(argv[1]) != "-t"  && // train
           std::string(argv[1]) != "-tc"    // train (continue from /net/best)
         ) )
    {
        std::cerr << "USAGE: train [ -t | -tc ] <data path> <workspace path>" << std::endl;
        exit(1);
    }
    
    using namespace fractal;
    
    Engine engine;
    Net net;

    net.Configure(engine, Net::RunType::train, argv[2], argv[3], std::string(argv[1]) == "-tc");
    net.Train();

    return 0;
}

