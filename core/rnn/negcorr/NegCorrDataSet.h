/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef NEGCORRDATASET_H_
#define NEGCORRDATASET_H_

#include "../TradeDataSet.h"
#include "Reshaper_n0.h"

class NegCorrDataSet : public TradeDataSet
{
public:
    NegCorrDataSet();

    const fractal::ChannelInfo GetChannelInfo(const unsigned long channelIdx) const override;
    
    void PutFrameData(const fractal::FLOAT *data, const unsigned long channelIdx, void *const frame) override;

private:
    sibyl::Reshaper_n0 reshaper;
};

#endif /* NEGCORRDATASET_H_ */