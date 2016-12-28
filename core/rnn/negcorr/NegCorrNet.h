/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef NEGCORRNET_H_
#define NEGCORRNET_H_

#include "../TradeNet.h"
#include "NegCorrDataSet.h"

namespace fractal
{

class NegCorrNet : public TradeNet<NegCorrDataSet>
{
public:
    void Train() override;
private:
    void ConfigureLayers() override;
};

}

#endif /* NEGCORRNET_H_ */