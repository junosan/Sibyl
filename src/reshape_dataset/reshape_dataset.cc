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

#include <iostream>
#include <string>

#include <rnn/regress/RegressDataSet.h>
#include <rnn/regress/Reshaper_v0.h>
using DataSet = RegressDataSet<sibyl::Reshaper_v0>;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "USAGE: reshape_dataset <raw/ref path> <out path>" << std::endl;
        exit(1);
    }
    
    std::string dataPath     (argv[1]),
                workspacePath(argv[2]);

    DataSet trainData,
            devData;

    // Copy .list files to workspacePath
    verify(0 == system(std::string("mkdir -p " + workspacePath).c_str()));
    verify(0 == system(std::string("cp " + dataPath + "/*.list " + workspacePath).c_str()));

    // Calculate whitening & mean matrix on train data
    trainData.ReadFileList(dataPath + "/train.list");
    if (false == trainData.Reshaper().ReadWhiteningMatrix(workspacePath + "/mean.matrix", workspacePath + "/whitening.matrix"))
    {
        std::cerr << "Calculating whitening & mean matrices... ";
        trainData.Reshaper().CalcWhiteningMatrix(workspacePath + "/mean.matrix", workspacePath + "/whitening.matrix");
        std::cerr << '\n';
    }
    verify(true == trainData.Reshaper().ReadWhiteningMatrix(workspacePath + "/mean.matrix", workspacePath + "/whitening.matrix"));
    
    devData.ReadFileList(dataPath + "/dev.list");
    verify(true == devData.Reshaper().ReadWhiteningMatrix(workspacePath + "/mean.matrix", workspacePath + "/whitening.matrix"));    
    
    // Reshape and store data in memory
    std::cerr << "Reshaping data...\n";
    std::cerr << "    train set: ";
    trainData.ReadData(/* verbose */ true);
    std::cerr << '\n';
    std::cerr << "      dev set: ";
    devData  .ReadData(/* verbose */ true);
    std::cerr << '\n';

    // Dump to files
    std::cerr << "Dumping data...\n";
    std::cerr << "    train set: ";
    trainData.DumpData(workspacePath, /* verbose */ true);
    std::cerr << '\n';
    std::cerr << "      dev set: ";
    devData  .DumpData(workspacePath, /* verbose */ true);
    std::cerr << '\n';

    std::cerr << "Done\n";

    return 0;
}