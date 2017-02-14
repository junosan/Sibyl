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

#ifndef SIBYL_UTIL_DISPPREFIX_H_
#define SIBYL_UTIL_DISPPREFIX_H_

#include <iostream>

#include "../sibyl_common.h"

namespace sibyl
{

// Prefix string returned by a function (e.g., timestamps) to any output message
// Use the global instance "dispPrefix": cout/err << dispPrefix << (desired content)
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