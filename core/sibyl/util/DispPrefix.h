#ifndef SIBYL_UTIL_DISPPREFIX_H_
#define SIBYL_UTIL_DISPPREFIX_H_

#include <iostream>

#include "../SibylCommon.h"

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

}

#endif /* SIBYL_UTIL_DISPPREFIX_H_ */