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

#include <new>

#include "sibyl_common.h"

namespace sibyl
{

class NetAgent
{
public:
    void SetVerbose(bool verbose_) { verbose = verbose_; }
    
    NetAgent() : verbose(false)
                { bufTCP = new char[kTCPBufSize];
                  bufMsg = new char[kTCPBufSize]; }
    ~NetAgent() { delete[] bufTCP;
                  delete[] bufMsg; }
protected:
    bool verbose;
    
    constexpr static const char *kTCPPassword = "sendorder";
    constexpr static       int   kTCPBacklog  = 8;
    constexpr static       int   kTCPBufSize  = (1 << 16);
    constexpr static       int   sock_fail    = -1; 
    
    char *bufTCP;
    char *bufMsg;
};

}

#endif /* SIBYL_NETAGENT_H_ */