#ifndef SIBYL_SECURITY_ELW_
#define SIBYL_SECURITY_ELW_

#include <cmath>

#include "Security.h"

namespace sibyl
{

template <class TItem>
class ELW : public TItem // derive from a specialized Security<container<TOrd>> (i.e., Item)
{
public:
    // virtuals from Security
    SecType Type  ()        const { return kSecELW;                     }
    INT     TckHi (INT p)   const { return p + 5;                       }
    INT     TckLo (INT p)   const { return (p - 5) * (p > 0);           }
    bool    ValidP(INT p)   const { return (p > 0) && ((p % 5) == 0);   }
    INT64   BFee  (INT64 r) const { return (INT64)std::round(r * dBF0); }
    INT64   SFee  (INT64 r) const { return (INT64)std::round(r * dSF0); }
    double  dBF   ()        const { return dBF0;                        }
    double  dSF   ()        const { return dSF0;                        }
    
    // additionals
    static FLOAT                          kospi200;
    constexpr static const std::ptrdiff_t szTh = 8;  // number of fields in theoretical Greeks
    std::array<FLOAT, szTh>               thr;
    void    SetInfo(OptType optType_, INT expiry_) {
        assert(((optType_ == kOptCall) || (optType_ == kOptPut)) && (expiry_ >= 0));
        optType = optType_; expiry = expiry_;
    }
    OptType CallPut() const { assert(optType != kOptNull); return optType; }
    INT     Expiry () const { assert(optType != kOptNull); return expiry;  }
    
    ELW() : thr{}, optType(kOptNull), expiry(0) {}
    ELW(OptType t, INT e) : thr{} { SetInfo(t, e); }
private:
    constexpr static const double dBF0 = 0.00015;
    constexpr static const double dSF0 = 0.00015;
    OptType optType;
    INT     expiry;
};
    
template <class TItem>
FLOAT ELW<TItem>::kospi200 = 0.0f;

}

#endif /* SIBYL_SECURITY_ELW_ */