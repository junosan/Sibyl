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

#ifndef CLASSIFDATASET_H_
#define CLASSIFDATASET_H_

#include "../TradeDataSet.h"

template <class TReshaper>
class ClassifDataSet : public TradeDataSet
{
public:
    ClassifDataSet();

    const fractal::ChannelInfo GetChannelInfo(const unsigned long channelIdx) const override;
    
    void PutFrameData(const fractal::FLOAT *data, const unsigned long channelIdx, void *const frame) override;

private:
    TReshaper reshaper;
};

template <class TReshaper>
ClassifDataSet<TReshaper>::ClassifDataSet() : reshaper(0, this, &fileList, &TradeDataSet::ReadRawFile) // reshaper(maxGTck, &, &, &)
{
    pReshaper = &reshaper;
    inputDim  = reshaper.GetInputDim();
    targetDim = reshaper.GetTargetDim();
}

template <class TReshaper>
const fractal::ChannelInfo ClassifDataSet<TReshaper>::GetChannelInfo(const unsigned long channelIdx) const
{
    fractal::ChannelInfo channelInfo;

    switch(channelIdx)
    {
        case CHANNEL_INPUT:
            channelInfo.dataType = fractal::ChannelInfo::DATATYPE_VECTOR;
            channelInfo.frameSize = inputDim;
            channelInfo.frameDim = inputDim;
            break;

        case CHANNEL_TARGET:
            channelInfo.dataType = fractal::ChannelInfo::DATATYPE_INDEX;
            channelInfo.frameSize = 1;
            channelInfo.frameDim = targetDim;
            break;

        case CHANNEL_SIG_NEWSEQ:
            channelInfo.dataType = fractal::ChannelInfo::DATATYPE_VECTOR;
            channelInfo.frameSize = 1;
            channelInfo.frameDim = 1;
            break;

        default:
            verify(false);
    }

    return channelInfo;
}

template <class TReshaper>
void ClassifDataSet<TReshaper>::PutFrameData(const fractal::FLOAT *data, const unsigned long channelIdx, void *const frame)
{
    typedef sibyl::Reshaper_p0::vecout_idx idx;

    switch(channelIdx)
    {
        case CHANNEL_INPUT:
            memcpy(frame, data, sizeof(fractal::FLOAT) * inputDim);
            break;

        case CHANNEL_TARGET:
            *reinterpret_cast<INT*>(frame) = (fractal::INT) ( idx::sell * (data[idx::sell] > 0.0f) +
                                                              idx::buy  * (data[idx::buy ] > 0.0f) );
            break;

        case CHANNEL_SIG_NEWSEQ:
            reinterpret_cast<fractal::FLOAT *>(frame)[0] = data[0];
            break;

        default:
            verify(false);
    }
}

#endif /* CLASSIFDATASET_H_ */