#ifndef SIBYL_CLIENT_NETCLIENT_H_
#define SIBYL_CLIENT_NETCLIENT_H_

#include <cstring>

#include "../NetAgent.h"
#include "Trader.h"

namespace sibyl
{

class NetClient : public NetAgent
{
public:
    int  Connect     (CSTR &addr, CSTR &port); // returns 0 for success, non-0 otherwise
    int  RecvNextTick();                                                 // returns 0 for success, non-0 otherwise
    void SendResponse();

    NetClient(Trader *ptr) : pTrader(ptr), sock(sock_fail) {}
private:
    Trader *pTrader;
    int sock;
};

int NetClient::Connect(CSTR &addr, CSTR &port)
{
    addrinfo hints, *ai;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    getaddrinfo(addr.c_str(), port.c_str(), &hints, &ai);
    int ret            = sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock_fail != ret) ret = connect(sock, ai->ai_addr, ai->ai_addrlen);
    freeaddrinfo(ai);
    
    if ( 0 == ret) send(sock, kTCPPassword.c_str(), kTCPPassword.size(), 0);
    else           std::cerr << strerror(errno) << std::endl;
    return ret;
}

int NetClient::RecvNextTick()
{   
    // Process packets until a full message is received
    std::size_t lenMark       = 3;
    const char  pcStartMark[] = "/*\n";
    const char  pcEndMark  [] = "*/\n";
    
    bufMsg[0] = '\0';
    bool isNewMsg = true;
    while (true)
    {
        memset(bufTCP, 0, kTCPBufSize);
        if (recv(sock, bufTCP, kTCPBufSize, 0) > 0) 
        {
            if (isNewMsg == true)
            {
                if (strncmp(bufTCP, pcStartMark, lenMark))
                    continue;
                else
                    isNewMsg = false;
            }
            std::size_t len = strlen(bufTCP);
            if ((len = strlen(bufMsg) + len) >= kTCPBufSize)
            {
                std::cerr << "NetClient: TCP input buffer overflow" << std::endl;
                return -1;
            }
            strcat(bufMsg, bufTCP);
            if ((len >= lenMark) && !strncmp(bufMsg + len - lenMark, pcEndMark, lenMark))
                break;
        }
        else
        {
            if (verbose) std::cout << "Server disconnected" << std::endl;
            close(sock);
            sock = sock_fail;
            return -1;
        }
    }
    if (verbose) std::cout << bufMsg << std::endl;
    
    if (0 != pTrader->ApplyMsgIn(bufMsg))
    {
        close(sock);
        sock = sock_fail;
        return -1;
    }
    return 0;
}

void NetClient::SendResponse()
{
    auto &msg = pTrader->BuildMsgOut(); 
    send(sock, msg.c_str(), msg.size(), 0);
    if (verbose) std::cout << msg << std::endl;
}

}

#endif  /* SIBYL_CLIENT_NETCLIENT_H_ */