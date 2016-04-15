#ifndef __TRADEDATASET_H__
#define __TRADEDATASET_H__

#include <vector>
#include <string>
#include <list>
#include <fstream>

#include <fractal/fractal.h>

#include "Reshaper_0.h"

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
    
    /* Reshaper */
    typedef sibyl::Reshaper_0 Reshaper; // TradeRnn also refers TradeDataSet::Reshaper
    Reshaper reshaper;
    
protected:
    static const unsigned long ReadRawFile(std::vector<fractal::FLOAT> &vec, const std::string &filename, TradeDataSet *pThis);
    const unsigned long ReadRefFile(std::vector<fractal::FLOAT> &vec, const std::string &filename);

    std::vector<std::string> fileList;
    std::vector<unsigned long> nFrame;

    std::vector<std::vector<fractal::FLOAT>> input;
    std::vector<std::vector<fractal::FLOAT>> target;

    std::vector<fractal::FLOAT> mean, stdev;

    unsigned long nSeq;
    unsigned long inputDim;
    unsigned long targetDim;
};

#endif /* __TRADEDATASET_H__ */

