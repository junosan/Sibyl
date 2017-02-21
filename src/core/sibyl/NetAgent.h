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

// Base class for NetServer and NetClient
class NetAgent
{
public:
    void SetVerbose(bool verbose_) { verbose = verbose_; }
    
    NetAgent() : verbose(false)
                { bufTCP = new char[kTCPBufSize];
                  bufMsg = new char[kTCPBufSize]; }
    NetAgent           (const NetAgent&) = delete;
    NetAgent& operator=(const NetAgent&) = delete;
    NetAgent           (NetAgent&&) = delete;
    NetAgent& operator=(NetAgent&&) = delete;
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