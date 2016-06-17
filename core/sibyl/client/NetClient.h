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

class NetClient : public NetAgent
{
public:
    int  Connect     (CSTR &addr, CSTR &port); // returns 0 for success, non-0 otherwise
    int  RecvNextTick();                       // returns 0 for success, non-0 otherwise
    void SendResponse();

    NetClient(Trader *ptr) : pTrader(ptr), sock(sock_fail) {}
private:
    Trader *pTrader;
    int sock;
};

}

#endif  /* SIBYL_CLIENT_NETCLIENT_H_ */