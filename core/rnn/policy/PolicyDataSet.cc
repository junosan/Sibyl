/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include "PolicyDataSet.h"

using namespace fractal;

PolicyDataSet::PolicyDataSet() : reshaper(0, this, &fileList, &TradeDataSet::ReadRawFile) // reshaper(maxGTck, &, &, &)
{
    pReshaper = &reshaper;
    inputDim  = reshaper.GetInputDim();
    targetDim = reshaper.GetTargetDim();
}

const ChannelInfo PolicyDataSet::GetChannelInfo(const unsigned long channelIdx) const
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
            channelInfo.dataType = ChannelInfo::DATATYPE_INDEX;
            channelInfo.frameSize = 1;
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

void PolicyDataSet::PutFrameData(const FLOAT *data, const unsigned long channelIdx, void *const frame)
{
    typedef sibyl::Reshaper_p0::vecout_idx idx;

    switch(channelIdx)
    {
        case CHANNEL_INPUT:
            memcpy(frame, data, sizeof(FLOAT) * inputDim);
            break;

        case CHANNEL_TARGET:
            *reinterpret_cast<INT*>(frame) = (INT) ( idx::sell * (data[idx::sell] > 0.0f) +
                                                     idx::buy  * (data[idx::buy ] > 0.0f) );
            break;

        case CHANNEL_SIG_NEWSEQ:
            reinterpret_cast<FLOAT *>(frame)[0] = data[0];
            break;

        default:
            verify(false);
    }
}