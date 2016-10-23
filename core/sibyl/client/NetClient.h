/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

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