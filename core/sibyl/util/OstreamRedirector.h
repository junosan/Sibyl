/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

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