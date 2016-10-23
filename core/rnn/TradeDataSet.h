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

// Reads training data set while processing data as defined by a Reshaper
class TradeDataSet : public fractal::DataSet
{
public:
    static constexpr unsigned long CHANNEL_INPUT      = 0;
    static constexpr unsigned long CHANNEL_TARGET     = 1;
    static constexpr unsigned long CHANNEL_SIG_NEWSEQ = 2;

    TradeDataSet();

    // Read list file and fill in fileList entries
    const unsigned long ReadFileList(const std::string &filename);

    // Actually read files and fill in nSeq, nFrame, input, target
    // Any data processing is performed by Reshaper 
    void ReadData();

    // TradeNet refers to the Reshaper used here for use from outside code
    // so that the same Reshaper used during training can be used for running
    sibyl::Reshaper& Reshaper() { verify(pReshaper != nullptr); return *pReshaper; }

    // Virtuals from DataSet
    const unsigned long GetNumChannel() const;
    virtual const fractal::ChannelInfo GetChannelInfo(const unsigned long channelIdx) const = 0;
    const unsigned long GetNumSeq() const;
    const unsigned long GetNumFrame(const unsigned long seqIdx) const;
    virtual void GetFrameData(const unsigned long seqIdx, const unsigned long channelIdx,
            const unsigned long frameIdx, void *const frame) = 0;

    // These 5 are unused at the moment
    void Normalize();
    void Normalize(const std::vector<fractal::FLOAT> &mean, const std::vector<fractal::FLOAT> &stdev);
    const std::vector<fractal::FLOAT> &GetMean();
    const std::vector<fractal::FLOAT> &GetStdev();
    const std::string &GetFilename(const unsigned long seqIdx) const { verify(seqIdx < nSeq); return fileList[seqIdx]; }

protected:
    static const unsigned long ReadRawFile(std::vector<fractal::FLOAT> &vec, const std::string &filename, TradeDataSet *pThis);
    const unsigned long ReadRefFile(std::vector<fractal::FLOAT> &vec, const std::string &filename);

    unsigned long nSeq;

    // Holds info for each sequence
    std::vector<std::string> fileList; // file name
    std::vector<unsigned long> nFrame; // # of frames = inner_vectors_below.size()
    std::vector<std::vector<fractal::FLOAT>> input;  // input  data (after Reshaper processing)
    std::vector<std::vector<fractal::FLOAT>> target; // target data (after Reshaper processing)

    // NOTE: These three must be initialized in the derived class' constructor
    sibyl::Reshaper* pReshaper;
    unsigned long inputDim;
    unsigned long targetDim;

    // Unused
    std::vector<fractal::FLOAT> mean, stdev;
};

#endif /* TRADEDATASET_H_ */

