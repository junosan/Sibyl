#ifndef SIBYL_UTIL_DISPPREFIX_H_
#define SIBYL_UTIL_DISPPREFIX_H_

#include <iostream>

#include "../sibyl_common.h"

namespace sibyl
{

class DispPrefix
{
public:
    void SetFunc(CSTR& (*func_)()) { func = func_; }
    bool IsNull() const { return func == nullptr; }

    friend std::ostream& operator<<(std::ostream &os, const DispPrefix &p) {
        return (p.func != nullptr ? os << p.func() : os);
    }

    DispPrefix() : func(nullptr) {}
protected:
    CSTR& (*func)();
};

extern DispPrefix dispPrefix;

}

#endif /* SIBYL_UTIL_DISPPREFIX_H_ */