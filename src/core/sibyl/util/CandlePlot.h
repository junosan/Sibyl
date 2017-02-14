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

#ifndef SIBYL_UTIL_CANDLEPLOT_H_
#define SIBYL_UTIL_CANDLEPLOT_H_

#include "../sibyl_common.h"

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

namespace sibyl
{

// Use as os << CandlePlot(vec, nline, ymin, ymax, xbin, title)
// Creates a candle plot in an area of width vec.size() / xbin (approx)
// and height nline, with title placed at the lower left corner
// Each candle is formed by finding starting/ending/min/max values from xbin
// items in vec
// Symbols
//     higher than ymax        '/'
//     lower  than ymin        '\\'
//     between starting/ending '░' (rising ; \u2591)
//                             '█' (falling; \u2588)
//                             '-' (no change)
//     between min/max         '|' (unless masked by above)

class CandlePlot
{
public:
    CandlePlot(const std::vector<float> &vec, std::size_t nline_, float ymin_, float ymax_, std::size_t xbin_, const std::string &title_)
        : pvec(&vec), nline(nline_), ymin(ymin_), ymax(ymax_), xbin(xbin_), title(title_) {
        verify(nline >= 2 && ymax > ymin && xbin > 0);
    }
    
    friend std::ostream& operator<<(std::ostream &os, const CandlePlot &bp) {
        const auto &vec        = *bp.pvec;
        const std::size_t xbin = bp.xbin;
        const std::size_t nx   = vec.size() / xbin + (vec.size() % xbin != 0 ? 1 : 0);
        const std::size_t ny   = bp.nline;
        const float ymin       = bp.ymin;
        const float ymax       = bp.ymax;
        const float interval   = (ymax - ymin) / (ny - 1);

        auto val_to_yidx = [&](float val) {
            if      (val > ymax + 0.5f * interval) return (int) ny;
            else if (val < ymin - 0.5f * interval) return (int) -1;
            else    return (int) std::floor((val - (ymin - 0.5f * interval)) / interval);
        };

        std::vector<std::string> y_str(ny, std::string());
        
        // to handle multibyte-ness of UTF-8 characters '░' & '█'
        std::vector<std::size_t> y_nsb(ny, 0); // current number of symbols
        std::size_t idx_post_title = bp.title.size(); // first index not screened by title in line #0

        auto add_symbol = [&](std::size_t yidx, std::size_t xidx, const std::string &s) {
            auto pad = (std::ptrdiff_t) xidx - (std::ptrdiff_t) y_nsb[yidx];
            if (pad > 0) {
                y_str[yidx] += std::string(pad, ' ');
                y_nsb[yidx] += pad;
            } else if (pad == -1) { // only used to overwrite last '|'
                y_str[yidx].pop_back();
                y_nsb[yidx] -= 1;
            } // will silently screw up if pad < -1, so don't do that
            y_str[yidx] += s;
            y_nsb[yidx] += 1;
            if (yidx == 0 && xidx < bp.title.size())
                idx_post_title += s.size() - 1; // track increased index due to multibyte characters
        };

        for (std::size_t i = 0; i < nx; ++i)
        {
            const auto it_first = std::begin(vec) + i * xbin;
            const auto it_last  = (i < nx - 1 ? it_first + xbin : std::end(vec)); // [first, last)
            auto yidx_min = val_to_yidx(*std::min_element(it_first, it_last));
            auto yidx_max = val_to_yidx(*std::max_element(it_first, it_last));
            if (yidx_min == (int) ny) // larger than all bins
                add_symbol(ny - 1, i, "/");
            else if (yidx_max == (int) -1) // smaller than all bins
                add_symbol(0, i, "\\");
            else
            {
                // min/max bars
                yidx_min = std::max(yidx_min, (int) 0     );
                yidx_max = std::min(yidx_max, (int) ny - 1);
                for (auto yidx = yidx_min; yidx <= yidx_max; ++yidx)
                    // cannot use multibyte here as it may need to be overwritten
                    add_symbol((std::size_t) yidx, i, "|");
                
                // starting/ending candles
                auto val_first = *it_first;
                auto val_last  = *(it_last - 1);
                yidx_min = val_to_yidx(std::min(val_first, val_last));
                yidx_max = val_to_yidx(std::max(val_first, val_last));

                auto symbol = (val_last > val_first ? "\u2591" :
                              (val_last < val_first ? "\u2588" :
                              "-"));

                for (auto yidx = yidx_min; yidx <= yidx_max; ++yidx)
                    if (yidx >= 0 && yidx < (int) ny)
                        add_symbol((std::size_t) yidx, i, symbol);
            }
        }
        
        if (y_str[0].size() > idx_post_title)
            y_str[0] = bp.title + y_str[0].substr(idx_post_title);
        else
            y_str[0] = bp.title;

        for (auto it = y_str.rbegin(), end = y_str.rend(); it != end; ++it)
        {
            if (it != y_str.rbegin()) os << '\n';
            os << *it;
        }
        
        return os;
    }
    
private:
    const std::vector<float> *pvec;
    std::size_t nline;
    float ymin, ymax;
    std::size_t xbin;
    std::string title;
};

}

#endif /* SIBYL_UTIL_CANDLEPLOT_H_ */