#ifndef SIBYL_NETAGENT_H_
#define SIBYL_NETAGENT_H_

#ifndef _WIN32
    #include <netdb.h>
    #include <unistd.h>
    #define close_socket(expression) close(expression)
#else
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define close_socket(expression) closesocket(expression)
#endif /* !_WIN32 */

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