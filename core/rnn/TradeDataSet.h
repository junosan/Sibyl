/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef TRADEDATASET_H_
#define TRADEDATASET_H_

#include <fractal/fractal.h>
#include <sibyl/time_common.h>
#include "Reshaper.h"

#include <vector>
#include <map>
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

    // Read list file and fill in fileList entries and nSeq
    const unsigned long ReadFileList(const std::string &filename);

    // TradeNet refers to the Reshaper used here for use from outside code
    // so that the same Reshaper used during training can be used for inference
    sibyl::Reshaper& Reshaper() { verify(pReshaper != nullptr); return *pReshaper; }

    // Virtuals from DataSet
    const unsigned long GetNumChannel() const;
    virtual const fractal::ChannelInfo GetChannelInfo(const unsigned long channelIdx) const = 0;
    const unsigned long GetNumSeq() const;
    const unsigned long GetNumFrame(const unsigned long seqIdx) const;
    void GetFrameData(const unsigned long seqIdx, const unsigned long channelIdx,
            const unsigned long frameIdx, void *const frame);
    
    // Put data to frame with appropriate conversion (specific to a derived class)
    virtual void PutFrameData(const fractal::FLOAT *data, const unsigned long channelIdx, void *const frame) = 0;

    const unsigned long ReadRawFile(std::vector<fractal::FLOAT> &vec, const std::string &filename);
    const unsigned long ReadRefFile(std::vector<fractal::FLOAT> &vec, const std::string &filename);

    // Unused
    // Actually read files and fill in nFrame, input, target
    // Any data processing is performed by Reshaper 
    // void ReadData();
    // const std::string &GetFilename(const unsigned long seqIdx) const { verify(seqIdx < nSeq); return fileList[seqIdx]; }

protected:
    unsigned long nSeq;

    // Holds info for each sequence
    std::vector<std::string> fileList; // file name
    // std::vector<unsigned long> nFrame; // # of frames = inner_vectors_below.size()
    // std::vector<std::vector<fractal::FLOAT>> input;  // input  data (after Reshaper processing)
    // std::vector<std::vector<fractal::FLOAT>> target; // target data (after Reshaper processing)

    constexpr static long nFrame = std::ceil((double) (6 * 3600 - 10 * 60) / sibyl::kTimeRates::secPerTick) - 1;
    std::map<unsigned long, std::vector<fractal::FLOAT>> input;  // buffered input  data (reshaped)
    std::map<unsigned long, std::vector<fractal::FLOAT>> target; // buffered target data (reshaped)
    
    // // for debugging (check that frameIdx is nondecreasing)
    // std::map<unsigned long, unsigned long> lastFrameIdx;

    // NOTE: These three must be initialized in the derived class' constructor
    sibyl::Reshaper* pReshaper;
    unsigned long inputDim;
    unsigned long targetDim;

};

#endif /* TRADEDATASET_H_ */

