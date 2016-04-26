#ifndef SIBYL_UTIL_BASICPLOT_H_
#define SIBYL_UTIL_BASICPLOT_H_

#include <iostream>
#include <vector>
#include <map>

#include "../sibyl_common.h"

namespace sibyl
{

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
        
        std::map<float, STR> plot;
        STR blankline(nx, ' ');
        for (std::size_t i = 0; i < ny; i++)
            plot[ymin + (i - 0.5f) * interval] = blankline; // store minimum value of each bin

        for (std::size_t i = 0; i < nx; i++)
        {
            if (vec[i] > ymax + 0.5f * interval) // larger than all bins
            {
                auto last = std::end(plot);
                (--last)->second[i] = '/';
            }
            else
            {
                auto ub = plot.upper_bound(vec[i]);
                if (ub == std::begin(plot)) // smaller than all bins
                    std::begin(plot)->second[i] = '\\';
                else
                    (--ub)->second[i] = '*';
            }
        }
        
        for (auto it = plot.rbegin(); it != plot.rend(); it++)
        {
            if (it != plot.rbegin()) os << '\n';
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