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

#ifndef SIBYL_UTIL_BASICPLOT_H_
#define SIBYL_UTIL_BASICPLOT_H_

#include <iostream>
#include <vector>
#include <map>

#include "../sibyl_common.h"

namespace sibyl
{

// Use as os << BasicPlot(vec, nline, ymin, ymax)
// Creates a very basic plot in an area of width vec.size() and height nline
// Symbols
//     higher than ymax '/'
//     lower  than ymin '\\'
//     otherwise        '*'
class BasicPlot
{
public:
    BasicPlot(const std::vector<float> &vec, std::size_t nline_, float ymin_, float ymax_)
        : pvec(&vec), nline(nline_), ymin(ymin_), ymax(ymax_) { verify(ymax > ymin); verify(nline >= 2); }
    
    friend std::ostream& operator<<(std::ostream &os, const BasicPlot &bp) {
        const auto &vec = *bp.pvec;
        const std::size_t nx  = vec.size();
        const std::size_t ny  = bp.nline;
        const float ymin      = bp.ymin;
        const float ymax      = bp.ymax;
        const float interval  = (ymax - ymin) / (ny - 1);
        
        std::map<float, STR> y_str;
        STR blankline(nx, ' ');
        for (std::size_t i = 0; i < ny; i++)
            y_str[ymin + (i - 0.5f) * interval] = blankline; // store minimum value of each bin

        for (std::size_t i = 0; i < nx; i++)
        {
            if (vec[i] > ymax + 0.5f * interval) // larger than all bins
                y_str.rbegin()->second[i] = '/';
            else
            {
                auto ub = y_str.upper_bound(vec[i]);
                if (ub == std::begin(y_str)) // smaller than all bins
                    ub->second[i] = '\\';
                else
                    (--ub)->second[i] = '*';
            }
        }
        
        for (auto it = y_str.rbegin(); it != y_str.rend(); it++)
        {
            if (it != y_str.rbegin()) os << '\n';
            os << it->second;
        }
        
        return os;
    }
    
private:
    const std::vector<float> *pvec;
    std::size_t nline;
    float ymin, ymax;
};

}

#endif /* SIBYL_UTIL_BASICPLOT_H_ */