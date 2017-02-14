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