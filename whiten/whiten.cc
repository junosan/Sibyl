#include <iostream>
#include <string>

#include <rnn/TradeDataSet.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "USAGE: whiten <raw list file>" << std::endl;
        exit(1);
    }
    
    std::string path(argv[1]);
    path.resize(path.find_last_of('/'));
    
    TradeDataSet dataset;
    dataset.ReadFileList(argv[1]);
    dataset.reshaper.CalcWhiteningMatrix(path + "/mean.matrix", path + "/whitening.matrix");
    dataset.reshaper.DispWhiteningMatrix();
    
    return 0;
}