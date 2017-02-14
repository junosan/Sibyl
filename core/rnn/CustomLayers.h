/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef FRACTAL_CUSTOMLAYERS_H_
#define FRACTAL_CUSTOMLAYERS_H_

#include <fractal/fractal.h>

namespace fractal
{

void AddLstmLayer_ForgetOneInit(Rnn &rnn,
                                const std::string &name,
                                const std::string &biasLayer,
                                const unsigned long delayAmount,
                                const unsigned long size,
                                const bool selfLoop,
                                const InitWeightParam &initWeightParam = InitWeightParam());

void AddResGateLayer(Rnn &rnn,
                     const std::string &name,
                     const std::string &biasLayer,
                     const unsigned long size);

void AddLstmLayer_LearnInit(Rnn &rnn,
                            const std::string &name,
                            const std::string &biasLayer,
                            const unsigned long delayAmount,
                            const unsigned long size,
                            const InitWeightParam &initWeightParam = InitWeightParam());

void AddLstmLayer_DSigmoidOut(Rnn &rnn,
                              const std::string &name,
                              const std::string &biasLayer,
                              const unsigned long delayAmount,
                              const unsigned long size,
                              const bool selfLoop,
                              const InitWeightParam &initWeightParam = InitWeightParam());

void AddOELstmLayer(Rnn &rnn,
                    const std::string &name,
                    const std::string &biasLayer,
                    const unsigned long delayAmount,
                    const unsigned long size,
                    const bool selfLoop,
                    const InitWeightParam &initWeightParam = InitWeightParam());

}

#endif /* FRACTAL_CUSTOMLAYERS_H_ */