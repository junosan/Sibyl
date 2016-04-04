#ifndef SIBYL_RESHAPER_RAW_H_
#define SIBYL_RESHAPER_RAW_H_

#include "Reshaper.h"

namespace sibyl
{

class Reshaper_raw : public Reshaper<ItemMem>
{
public:
    Reshaper_raw(unsigned long maxGTck_,
                 TradeDataSet *pTradeDataSet_,
                 std::vector<std::string> *pFileList_,
                 const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, CSTR&, TradeDataSet*));

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2VecIn(FLOAT *vec, const ItemState &state);
};

}

#endif /* SIBYL_RESHAPER_RAW_H_ */