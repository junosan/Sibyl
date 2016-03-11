#ifndef SIBYL_NETAGENT_H_
#define SIBYL_NETAGENT_H_

#include <netdb.h>
#include <unistd.h>

#include "SibylCommon.h"

namespace sibyl
{

class NetAgent
{
public:
    void SetVerbose(bool verbose_) { verbose = verbose_; }
    
    NetAgent() : verbose(false) {}
protected:
    bool verbose;
    
    constexpr static const int sock_fail = -1; 
    
    static char bufTCP[kTCPBufSize];
    static char bufMsg[kTCPBufSize];
};

}

#endif /* SIBYL_NETAGENT_H_ */