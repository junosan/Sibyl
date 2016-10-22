/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_UTIL_CANDLEPLOT_H_
#define SIBYL_UTIL_CANDLEPLOT_H_

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

#include "../sibyl_common.h"

namespace sibyl
{

// Use as os << CandlePlot(vec, nline, ymin, ymax, xbin, title)
// Creates a candle plot in an area of width vec.size() / xbin (approx) and height nline
// Title is placed at lower left corner
// Each candle is formed by finding starting/ending/min/max values from xbin items in vec
// Symbols
//     higher than ymax        '/'
//     lower  than ymin        '\\'
//     between starting/ending '░' (rising), '█' (falling), or '-' (no change)
//     between min/max         '|' (unless masked by '░', '█', or '-')
// 
// Note: requires os.imbue(std::locale("en_US.UTF-8")) to properly display candles
class CandlePlot
{
public:
    CandlePlot(const std::vector<float> &vec, std::size_t nline_, float ymin_, float ymax_, std::size_t xbin_, const std::wstring &title_)
        : pvec(&vec), nline(nline_), ymin(ymin_), ymax(ymax_), xbin(xbin_), title(title_) {
        verify(nline >= 2 && ymax > ymin && xbin > 0);
    }
    
    friend std::wostream& operator<<(std::wostream &os, const CandlePlot &bp) {
        const auto &vec        = *bp.pvec;
        const std::size_t xbin = bp.xbin;
        const std::size_t nx   = vec.size() / xbin + (vec.size() % xbin != 0 ? 1 : 0);
        const std::size_t ny   = bp.nline;
        const float ymin       = bp.ymin;
        const float ymax       = bp.ymax;
        const float interval   = (ymax - ymin) / (ny - 1);

        std::vector<std::wstring> y_str(ny, std::wstring(nx, ' '));
        auto val_to_yidx = [&](float val) {
            if      (val > ymax + 0.5f * interval) return (int) ny;
            else if (val < ymin - 0.5f * interval) return (int) -1;
            else    return (int) std::floor((val - (ymin - 0.5f * interval)) / interval);
        };

        for (std::size_t i = 0; i < nx; ++i)
        {
            const auto it_first = std::begin(vec) + i * xbin;
            const auto it_last  = (i < nx - 1 ? it_first + xbin : std::end(vec)); // [first, last)
            auto yidx_min = val_to_yidx(*std::min_element(it_first, it_last));
            auto yidx_max = val_to_yidx(*std::max_element(it_first, it_last));
            if      (yidx_min == (int) ny) // larger than all bins
                y_str.back ()[i] = '/';
            else if (yidx_max == (int) -1) // smaller than all bins
                y_str.front()[i] = '\\';
            else
            {
                // min/max bars
                yidx_min = std::max(yidx_min, (int) 0     );
                yidx_max = std::min(yidx_max, (int) ny - 1);
                for (auto yidx = yidx_min; yidx <= yidx_max; ++yidx)
                    y_str[(std::size_t) yidx][i] = '|';
                
                // starting/ending candles
                auto val_first = *it_first;
                auto val_last  = *(it_last - 1);
                wchar_t sign = (val_last > val_first ? L'░' : (val_last < val_first ? L'█' : '-'));

                yidx_min = val_to_yidx(std::min(val_first, val_last));
                yidx_max = val_to_yidx(std::max(val_first, val_last));

                for (auto yidx = yidx_min; yidx <= yidx_max; ++yidx)
                    if (yidx >= 0 && yidx < (int) ny) y_str[(std::size_t) yidx][i] = sign;
            }
        }
        
        if (bp.title.size() < y_str[0].size()) {
            for (std::size_t i = 0, end = bp.title.size(); i != end; ++i)
                y_str[0][i] = bp.title[i];
        } else
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
    std::wstring title;
};

}

#endif /* SIBYL_UTIL_CANDLEPLOT_H_ */