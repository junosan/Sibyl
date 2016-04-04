
#include "Reshaper_delta_whiten.h"

namespace sibyl
{

Reshaper_delta_whiten::Reshaper_delta_whiten(unsigned long maxGTck_,
                                             TradeDataSet *pTradeDataSet_,
                                             std::vector<std::string> *pFileList_,
                                             const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, CSTR&, TradeDataSet*))
                                             : Reshaper(maxGTck_, pTradeDataSet_, pFileList_, ReadRawFile_)
{
    inputDim  = 44;
}

void Reshaper_delta_whiten::State2VecIn(FLOAT *vec, const ItemState &state)
{
    const long interval = kTimeTickSec; // seconds
    const long T = (const long)(std::ceil((6 * 3600 - 10 * 60)/interval) - 1);
    
    auto iItems = items.find(state.code);
    if (iItems == std::end(items))
    {
        auto it_bool = items.insert(std::make_pair(state.code, ItemMem_delta_whiten()));
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
    
    // ps1 - pb1
    vec[idxInput++] = ReshapePrice(state.tbr[idxPs1].p) - ReshapePrice(state.tbr[idxPb1].p);
    
    // // tbpr(1:20)
    // for (std::size_t idx = 0; idx < (std::size_t)szTb; idx++)
    //     vec[idxInput++] = ReshapePrice(state.tbr[idx].p) - ReshapePrice(i.initPr);
    
    // tbqr(1:20)
    for (std::size_t idx = 0; idx < (std::size_t)szTb; idx++)
        vec[idxInput++] = ReshapeQuant(state.tbr[idx].q);
    
    // delta_tbqr(1:20)
    for (std::size_t idx = 0; idx < (std::size_t)szTb; idx++)
    {
        INT delta = state.tbr[idx].q;
        for (std::size_t idxL = 0; idxL < (std::size_t)szTb; idxL++)
        {
            if (state.tbr[idx].p == i.lastTb[idxL].p)
            {
                if ( (idx <= idxPs1 && idxL <= idxPs1) || 
                     (idx >= idxPb1 && idxL >= idxPb1) )
                    delta = state.tbr[idx].q - i.lastTb[idxL].q;
                else
                    delta = state.tbr[idx].q + i.lastTb[idxL].q;
                break;
            }
        }
        vec[idxInput++] = ReshapeQuant(delta);
    }
    i.lastTb = state.tbr;
    
    verify(inputDim == idxInput);
    
    WhitenVector(vec); // this alters vector only if matrices are initialized
}

}
