/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include <iostream>
#include <string>

#include <rnn/value/ValueDataSet.h>
typedef ValueDataSet DataSet;

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