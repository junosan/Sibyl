/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_SECURITY_KOSPI_
#define SIBYL_SECURITY_KOSPI_

#include <cmath>

#include "../Security.h"

namespace sibyl
{

template <class TItem>     
class KOSPI : public TItem // derive from a specialized Security<TOrder> (i.e., Item)
{
public:
    // virtuals from Security
    SecType Type  ()        const { return SecType::KOSPI;                 }
    INT     TckHi (INT p)   const { 
        return  p + (                 (p <    1000) *      1 +
                      (p >=   1000) * (p <    5000) *      5 +
                      (p >=   5000) * (p <   10000) *     10 +
                      (p >=  10000) * (p <   50000) *     50 +
                      (p >=  50000) * (p <  100000) *    100 +
                      (p >= 100000) * (p <  500000) *    500 +
                      (p >= 500000)                 *   1000 );            }
    INT     TckLo (INT p)   const {
        return (p - (                 (p <=   1000) *      1 +
                      (p >    1000) * (p <=   5000) *      5 +
                      (p >    5000) * (p <=  10000) *     10 +
                      (p >   10000) * (p <=  50000) *     50 +
                      (p >   50000) * (p <= 100000) *    100 +
                      (p >  100000) * (p <= 500000) *    500 +
                      (p >  500000)                 *   1000 )) * (p > 0); }
    bool    ValidP(INT p)   const {
        INT q =                       (p <    1000) *      1 +
                      (p >=   1000) * (p <    5000) *      5 +
                      (p >=   5000) * (p <   10000) *     10 +
                      (p >=  10000) * (p <   50000) *     50 +
                      (p >=  50000) * (p <  100000) *    100 +
                      (p >= 100000) * (p <  500000) *    500 +
                      (p >= 500000)                 *   1000 ;
        INT b =                       (p <    1000) *      0 +
                      (p >=   1000) * (p <    5000) *   1000 +
                      (p >=   5000) * (p <   10000) *   5000 +
                      (p >=  10000) * (p <   50000) *  10000 +
                      (p >=  50000) * (p <  100000) *  50000 +
                      (p >= 100000) * (p <  500000) * 100000 +
                      (p >= 500000)                 * 500000 ;
        return (p > 0) && (((p - b) % q) == 0);                            }
    INT64   BFee  (INT64 r) const { return (INT64)std::round(r * dBF0);    }
    INT64   SFee  (INT64 r) const { return (INT64)std::round(r * dSF0) +
                                           (INT64)std::floor(r * dST0) +
                                           (INT64)std::floor(r * dST1);    }
    double  dBF   ()        const { return dBF0;                           }
    double  dSF   ()        const { return dSF0 + dST0 + dST1;             }
   
private:
    constexpr static const double dBF0 = 0.00015;
    constexpr static const double dSF0 = 0.00015;
    constexpr static const double dST0 = 0.0015;
    constexpr static const double dST1 = 0.0015;
};

}

#endif /* SIBYL_SECURITY_KOSPI_ */