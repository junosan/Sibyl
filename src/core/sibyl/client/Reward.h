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

#ifndef SIBYL_CLIENT_REWARD_H_
#define SIBYL_CLIENT_REWARD_H_

#include <array>

#include "../sibyl_common.h"

namespace sibyl
{

class Reward
{
public:
    struct {
        FLOAT s;
        FLOAT b;
    } G0;
    struct Gn {
        FLOAT s;
        FLOAT b;
        FLOAT cs;
        FLOAT cb;
    };
    std::array<Gn, idx::tckN> G;
    
    Reward& operator+=(const Reward &r) {
        G0.s += r.G0.s;
        G0.b += r.G0.b;
        for (std::size_t i = 0; i < (std::size_t) idx::tckN; i++) {
            G[i].s  += r.G[i].s;
            G[i].b  += r.G[i].b;
            G[i].cs += r.G[i].cs;
            G[i].cb += r.G[i].cb;
        }
        return *this;
    }
    Reward& operator*=(FLOAT r) {
        G0.s *= r;
        G0.b *= r;
        for (std::size_t i = 0; i < (std::size_t) idx::tckN; i++) {
            G[i].s  *= r;
            G[i].b  *= r;
            G[i].cs *= r;
            G[i].cb *= r;
        }
        return *this;
    }
    void SetZero() { G0 = {}; G = {}; }
    Reward() { SetZero(); }
};

}

#endif /* SIBYL_CLIENT_REWARD_H_ */