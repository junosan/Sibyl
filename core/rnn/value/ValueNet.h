/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef VALUENET_H_
#define VALUENET_H_

#include "../TradeNet.h"
#include "ValueDataSet.h"

namespace fractal
{

class ValueNet : public TradeNet<ValueDataSet>
{
public:
    void Train() override;
private:
    void ConfigureLayers() override;
};

}

#endif /* VALUENET_H_ */