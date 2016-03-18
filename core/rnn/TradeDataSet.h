#ifndef __TRADEDATASET_H__
#define __TRADEDATASET_H__

#include <vector>
#include <string>
#include <list>
#include <fstream>

#include <fractal/fractal.h>
#include <Eigen/Eigenvalues>

#include "Reshaper_delta.h"

class TradeDataSet : public fractal::DataSet
{
public:
    static const unsigned long CHANNEL_INPUT;
    static const unsigned long CHANNEL_TARGET;
    static const unsigned long CHANNEL_SIG_NEWSEQ;

    TradeDataSet();

    const unsigned long ReadFileList(const std::string &filename);
    void ReadData();
    void Normalize();
    void Normalize(const std::vector<fractal::FLOAT> &mean, const std::vector<fractal::FLOAT> &stdev);

    const std::vector<fractal::FLOAT> &GetMean();
    const std::vector<fractal::FLOAT> &GetStdev();

    const unsigned long GetNumChannel() const;
    const fractal::ChannelInfo GetChannelInfo(const unsigned long channelIdx) const;
    const unsigned long GetNumSeq() const;
    const unsigned long GetNumFrame(const unsigned long seqIdx) const;

    const std::string &GetFilename(const unsigned long seqIdx) const { verify(seqIdx < nSeq); return fileList[seqIdx]; }

    void GetFrameData(const unsigned long seqIdx, const unsigned long channelIdx,
            const unsigned long frameIdx, void *const frame);
    
    ////////////////
    sibyl::Reshaper_delta reshaper;
    bool ReadWhiteningMatrix(const std::string &filename_mean, const std::string &filename_whitening);
    void CalcWhiteningMatrix(const std::string &filename_mean, const std::string &filename_whitening);
    
protected:
    const unsigned long ReadRawFile(std::vector<fractal::FLOAT> &vec, const std::string &filename);
    const unsigned long ReadRefFile(std::vector<fractal::FLOAT> &vec, const std::string &filename);

    std::vector<std::string> fileList;
    std::vector<unsigned long> nFrame;

    std::vector<std::vector<fractal::FLOAT>> input;
    std::vector<std::vector<fractal::FLOAT>> target;

    std::vector<fractal::FLOAT> mean, stdev;

    unsigned long nSeq;
    unsigned long inputDim;
    unsigned long targetDim;
    
    ////////////////
    typedef float EScalar;
    typedef Eigen::Matrix<EScalar, Eigen::Dynamic, Eigen::Dynamic> EMatrix;
    
    EMatrix matMean, matWhitening;
    
    template<class Matrix>
    bool Eigen_write_binary(const std::string &filename, const Matrix &matrix);
    template<class Matrix>
    bool Eigen_read_binary(const std::string &filename, Matrix &matrix);
};

template<class Matrix>
bool TradeDataSet::Eigen_write_binary(const std::string &filename, const Matrix &matrix)
{
    std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    typename Matrix::Index rows = matrix.rows(), cols = matrix.cols();
    out.write((char*) (&rows), sizeof(typename Matrix::Index));
    out.write((char*) (&cols), sizeof(typename Matrix::Index));
    out.write((char*) matrix.data(), rows * cols * sizeof(typename Matrix::Scalar));
    return out.is_open();
}

template<class Matrix>
bool TradeDataSet::Eigen_read_binary(const std::string &filename, Matrix &matrix)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    typename Matrix::Index rows = 0, cols = 0;
    in.read((char*) (&rows), sizeof(typename Matrix::Index));
    in.read((char*) (&cols), sizeof(typename Matrix::Index));
    matrix.resize(rows, cols);
    in.read((char *) matrix.data(), rows * cols * sizeof(typename Matrix::Scalar));
    return in.is_open();
}

#endif /* __TRADEDATASET_H__ */

