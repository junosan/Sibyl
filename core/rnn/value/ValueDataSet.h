/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef VALUEDATASET_H_
#define VALUEDATASET_H_

#include "../TradeDataSet.h"
#include "Reshaper_v0.h"

class ValueDataSet : public TradeDataSet
{
public:
    ValueDataSet();

    const fractal::ChannelInfo GetChannelInfo(const unsigned long channelIdx) const override;
    
    void GetFrameData(const unsigned long seqIdx, const unsigned long channelIdx,
            const unsigned long frameIdx, void *const frame) override;

private:
    sibyl::Reshaper_v0 reshaper;
};

#endif /* VALUEDATASET_H_ */