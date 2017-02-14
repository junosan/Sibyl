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

#ifndef SIBYL_UTIL_OSTREAMREDIRECTOR_H_
#define SIBYL_UTIL_OSTREAMREDIRECTOR_H_

#include <iostream>
#include <fstream>
#include <string>

#include "../sibyl_common.h"

namespace sibyl
{

// Redirect cout, cerr to files for logging in GUI environment
// Instantiate in the longest living scope in an application
// Redirecting the same stream multiple times is undefined behavior
class OstreamRedirector
{
public:
    void Redirect(std::ostream &os, CSTR &filename) {
        if (ofs.is_open() == true) ofs.close();
        ofs.open(filename, std::ios::trunc);
        verify(ofs.is_open() == true);
        pos = &os;
        psb = os.rdbuf(ofs.rdbuf()); // save and replace streambuf
        verify(psb != nullptr);
    }

    OstreamRedirector() : pos(nullptr), psb(nullptr) {}
    OstreamRedirector(std::ostream &os, CSTR &filename) { Redirect(os, filename); }
    ~OstreamRedirector() { if (pos != nullptr) pos->rdbuf(psb); } // restore streambuf
protected:
    std::ofstream   ofs;
    std::ostream   *pos;
    std::streambuf *psb;
};

}

#endif /* SIBYL_UTIL_OSTREAMREDIRECTOR_H_ */