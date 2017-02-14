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

#ifndef SIBYL_CLIENT_NETCLIENT_H_
#define SIBYL_CLIENT_NETCLIENT_H_

#include "../NetAgent.h"
#include "Trader.h"

namespace sibyl
{

// Implement a TCP client and relay messages between Trader and the server
class NetClient : public NetAgent
{
public:
    // Connect to server and send password
    // Returns 0 for success, non-0 otherwise
    int Connect(CSTR &addr, CSTR &port);
    
    // Block until message from server arrives and call Trader::ApplyMsgIn
    // Returns 0 for success, non-0 otherwise
    int RecvNextTick();

    // Call Trader::BuildMsgOut and send the built message to server
    void SendResponse();

    NetClient(Trader *ptr) : pTrader(ptr), sock(sock_fail) {}
private:
    Trader *pTrader;
    int sock;
};

}

#endif  /* SIBYL_CLIENT_NETCLIENT_H_ */