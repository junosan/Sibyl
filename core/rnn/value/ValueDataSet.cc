/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include "ValueDataSet.h"

using namespace fractal;

ValueDataSet::ValueDataSet() : reshaper(1, this, &fileList, &ReadRawFile) // reshaper(maxGTck, &, &, &)
{
    pReshaper = &reshaper;
    inputDim  = reshaper.GetInputDim();
    targetDim = reshaper.GetTargetDim();
}

const ChannelInfo ValueDataSet::GetChannelInfo(const unsigned long channelIdx) const
{
    ChannelInfo channelInfo;

    switch(channelIdx)
    {
        case CHANNEL_INPUT:
            channelInfo.dataType = ChannelInfo::DATATYPE_VECTOR;
            channelInfo.frameSize = inputDim;
            channelInfo.frameDim = inputDim;
            break;

        case CHANNEL_TARGET:
            channelInfo.dataType = ChannelInfo::DATATYPE_VECTOR;
            channelInfo.frameSize = targetDim;
            channelInfo.frameDim = targetDim;
            break;

        case CHANNEL_SIG_NEWSEQ:
            channelInfo.dataType = ChannelInfo::DATATYPE_VECTOR;
            channelInfo.frameSize = 1;
            channelInfo.frameDim = 1;
            break;

        default:
            verify(false);
    }

    return channelInfo;
}

void ValueDataSet::GetFrameData(const unsigned long seqIdx, const unsigned long channelIdx,
        const unsigned long frameIdx, void *const frame)
{
    verify(seqIdx < nSeq);
    verify(frameIdx < nFrame[seqIdx]);

    switch(channelIdx)
    {
        case CHANNEL_INPUT:
            memcpy(frame, input[seqIdx].data() + inputDim * frameIdx, sizeof(FLOAT) * inputDim);
            break;

        case CHANNEL_TARGET:
            memcpy(frame, target[seqIdx].data() + targetDim * frameIdx, sizeof(FLOAT) * targetDim);
            break;

        case CHANNEL_SIG_NEWSEQ:
            reinterpret_cast<FLOAT *>(frame)[0] = (FLOAT) (frameIdx == 0);
            break;

        default:
            verify(false);
    }
}