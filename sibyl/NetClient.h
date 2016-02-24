#ifndef SIBYL_NETCLIENT_H_
#define SIBYL_NETCLIENT_H_

#include <fstream>
#include <cstring>

#include <netdb.h>
#include <unistd.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <sys/stat.h>
// #include <arpa/inet.h>

#include "Portfolio.h"

namespace sibyl
{

class NetClient {
public:
    void SetPortfolio(Portfolio *p_) { p = p_;             }
    void SetVerbose  (bool verbose_) { verbose = verbose_; }
    
    int  Connect     (const std::string &addr, const std::string &port); // returns 0 for success, non-0 otherwise
    int  RecvNextTick();                                                 // returns 0 for success, non-0 otherwise
    void SendResponse();

    NetClient() : p(nullptr), sock(-1), verbose(false) {}
private:
    Portfolio *p;
    int sock;
    bool verbose;
    
    static char bufTCP[kTCPBufSize];
    static char bufMsg[kTCPBufSize];
};

char NetClient::bufTCP[kTCPBufSize];
char NetClient::bufMsg[kTCPBufSize];

int NetClient::Connect(const std::string &addr, const std::string &port)
{
    assert(p != nullptr);
    
    addrinfo hints, *ai;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    getaddrinfo(addr.c_str(), port.c_str(), &hints, &ai);
    int ret     = sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (-1 != ret) ret = connect(sock, ai->ai_addr, ai->ai_addrlen);
    freeaddrinfo(ai);
    
    if ( 0 == ret) send(sock, kTCPPassword.c_str(), kTCPPassword.size(), 0);
    else           std::cerr << strerror(errno) << std::endl;
    return ret;
}

int NetClient::RecvNextTick()
{
    assert(sock != -1); // must call Connect(const char *, const char *) first
    
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
            sock = -1;
            return -1;
        }
    }
    if (verbose) std::cout << bufMsg << std::endl;
    
    if (0 != p->ReadMsgIn(bufMsg))
    {
        close(sock);
        sock = -1;
        return -1;
    }
    return 0;
}

void NetClient::SendResponse()
{
    assert(sock != -1);
    
    std::string msg;
    p->BuildMsgOut(msg); 
    send(sock, msg.c_str(), msg.size(), 0);
    if (verbose) std::cout << msg << std::endl;
}

}

#endif  /* SIBYL_NETCLIENT_H_ */