/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef TRADEDATASET_H_
#define TRADEDATASET_H_

#include <fractal/fractal.h>
#include "Reshaper.h"

#include <vector>
#include <string>
#include <list>
#include <fstream>

class TradeDataSet : public fractal::DataSet
{
public:
    static constexpr unsigned long CHANNEL_INPUT      = 0;
    static constexpr unsigned long CHANNEL_TARGET     = 1;
    static constexpr unsigned long CHANNEL_SIG_NEWSEQ = 2;

    TradeDataSet();

    const unsigned long ReadFileList(const std::string &filename);
    void ReadData();
    void Normalize();
    void Normalize(const std::vector<fractal::FLOAT> &mean, const std::vector<fractal::FLOAT> &stdev);

    const std::vector<fractal::FLOAT> &GetMean();
    const std::vector<fractal::FLOAT> &GetStdev();

    const unsigned long GetNumChannel() const;
    virtual const fractal::ChannelInfo GetChannelInfo(const unsigned long channelIdx) const = 0;
    const unsigned long GetNumSeq() const;
    const unsigned long GetNumFrame(const unsigned long seqIdx) const;

    const std::string &GetFilename(const unsigned long seqIdx) const { verify(seqIdx < nSeq); return fileList[seqIdx]; }

    virtual void GetFrameData(const unsigned long seqIdx, const unsigned long channelIdx,
            const unsigned long frameIdx, void *const frame) = 0;
    
    sibyl::Reshaper& Reshaper() { verify(pReshaper != nullptr); return *pReshaper; }
    
protected:
    static const unsigned long ReadRawFile(std::vector<fractal::FLOAT> &vec, const std::string &filename, TradeDataSet *pThis);
    const unsigned long ReadRefFile(std::vector<fractal::FLOAT> &vec, const std::string &filename);

    std::vector<std::string> fileList;
    std::vector<unsigned long> nFrame;

    std::vector<std::vector<fractal::FLOAT>> input;
    std::vector<std::vector<fractal::FLOAT>> target;

    std::vector<fractal::FLOAT> mean, stdev;

    unsigned long nSeq;

    /* These three must be initialized in the derived class' constructor */
    sibyl::Reshaper* pReshaper;
    unsigned long inputDim;
    unsigned long targetDim;
};

#endif /* TRADEDATASET_H_ */

