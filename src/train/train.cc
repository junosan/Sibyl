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

#include <string>
#include <iostream>

#include <fractal/fractal.h>

#include <rnn/regress/Reshaper_v0.h>
#include <rnn/regress/VanillaNet.h>
using Net = fractal::VanillaNet<sibyl::Reshaper_v0>;

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
