
#include "Reshaper_raw.h"

namespace sibyl
{

Reshaper_raw::Reshaper_raw(unsigned long maxGTck_,
                           TradeDataSet *pTradeDataSet_,
                           std::vector<std::string> *pFileList_,
                           const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, const std::string&, TradeDataSet*))
                           : Reshaper(maxGTck_, pTradeDataSet_, pFileList_, ReadRawFile_)
{
    inputDim  = 43;
}

void Reshaper_raw::State2VecIn(FLOAT *vec, const ItemState &state)
{
    const long interval = 10; // seconds
    const long T = (const long)(std::ceil((6 * 3600 - 10 * 60)/interval) - 1);
    
    auto iItems = items.find(state.code);
    if (iItems == std::end(items))
    {
        auto it_bool = items.insert(std::make_pair(state.code, ItemMem()));
        verify(it_bool.second == true);
        iItems = it_bool.first;
        iItems->second.initPr = state.pr;
    }
    auto &i = iItems->second; // reference to current ItemMem
    
    unsigned long idxInput = 0;
    
    // t
    vec[idxInput++] = (FLOAT) state.time / (interval * T);
    
    // pr
    vec[idxInput++] = ReshapePrice(state.pr) - ReshapePrice(i.initPr);
    
    // qr
    vec[idxInput++] = ReshapeQuant((INT) state.qr);
    
    // tbpr(1:20)
    for (std::size_t idx = 0; idx < (std::size_t)szTb; idx++)
        vec[idxInput++] = ReshapePrice(state.tbr[idx].p) - ReshapePrice(i.initPr);
    
    // tbqr(1:20)
    for (std::size_t idx = 0; idx < (std::size_t)szTb; idx++)
        vec[idxInput++] = ReshapeQuant(state.tbr[idx].q);
    
    verify(inputDim == idxInput);
    
    WhitenVector(vec); // this alters vector only if matrices are initialized
}

}
