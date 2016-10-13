/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef POLICYDATASET_H_
#define POLICYDATASET_H_

#include "../TradeDataSet.h"
#include "Reshaper_p0.h"

class PolicyDataSet : public TradeDataSet
{
public:
    PolicyDataSet();

    const fractal::ChannelInfo GetChannelInfo(const unsigned long channelIdx) const override;
    
    void GetFrameData(const unsigned long seqIdx, const unsigned long channelIdx,
            const unsigned long frameIdx, void *const frame) override;

private:
    sibyl::Reshaper_p0 reshaper;
};

#endif /* POLICYDATASET_H_ */