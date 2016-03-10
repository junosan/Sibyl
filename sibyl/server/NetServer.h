#ifndef SIBYL_SERVER_NETSERVER_H_
#define SIBYL_SERVER_NETSERVER_H_

#include "../NetAgent.h"
#include "Broker.h"

namespace sibyl
{

template <class TOrder, class TItem>
class NetServer : public NetAgent
{
public:
    void Run (CSTR &port, bool reconnectable = false);
    
    NetServer(Broker<TOrder, TItem> *pBroker_) : pBroker(pBroker_),
                                                 sock_serv(sock_fail), sock_conn(sock_fail) {} 
private:
    int  Initialize   (CSTR &port); // returns non-0 to signal error
    void SendMsgOut   ();
    int  RecvMsgIn    ();           // returns non-0 to signal client disconnection
    int  DisplayString(CSTR &disp, bool isError = false, int errno_ = 0);
    
    Broker<TOrder, TItem> *pBroker;
    
    int sock_serv;
    int sock_conn;
};

template <class TOrder, class TItem>
void NetServer<TOrder, TItem>::Run(CSTR &port, bool reconnectable)
{
    if (0 != Initialize(port)) return;
    while (true)
    {
        if  (0     != pBroker->AdvanceTick())          break;
        if  (false == pBroker->IsWoken    ())          SendMsgOut();
        if ((false == pBroker->IsWoken    ()) && (0 != RecvMsgIn())) break; 
    }
}

template <class TOrder, class TItem>
int NetServer<TOrder, TItem>::Initialize(CSTR &port)
{
    // Create server socket
    if (sock_fail == (sock_serv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)))
        return   DisplayString("[Fail] Create socket", true, errno);
    if (verbose) DisplayString("[Done] Create socket");
    
    // Prevent socket's address being blocked after app terminates
    int iOne = 1;
    if (sock_fail == setsockopt(sock_serv, SOL_SOCKET, SO_REUSEADDR, &iOne, sizeof(iOne)))
        return   DisplayString("[Fail] Change socket option", true, errno); 
    if (verbose) DisplayString("[Done] Change socket option");

    // Bind socket to address and listen
    struct sockaddr_in addr_serv;    
    memset(&addr_serv, 0, sizeof(addr_serv));
    addr_serv.sin_family      = AF_INET;
    addr_serv.sin_addr.s_addr = INADDR_ANY;
    addr_serv.sin_port        = htons(std::stoi(port));

    if (sock_fail == bind(sock_serv, (struct sockaddr *)&addr_serv, sizeof(addr_serv)))
        return   DisplayString("[Fail] Bind address to socket", true, errno);
    if (verbose) DisplayString("[Done] Bind address to socket");
        
    if (sock_fail == listen(sock_serv, kTCPBacklog))
        return   DisplayString("[Fail] Listen on socket", true, errno);
    if (verbose) DisplayString("[Done] Listen on socket");

    // Establish a connection and query password
    struct sockaddr_in addr_clnt;
    socklen_t szAddr = sizeof(addr_clnt);
    
    if(sock_fail == (sock_conn = accept(sock_serv, (struct sockaddr *)&addr_clnt, &szAddr)))
        return DisplayString("[Fail] Accept client", true, errno);
    else
    {
        memset(bufTCP, 0, kTCPBufSize);
        if (recv(sock_conn, bufTCP, kTCPBufSize, 0) > 0)
        {
            char *pc = strpbrk(bufTCP, "\r\n");
            if (pc != NULL) *pc = '\0';
            if (kTCPPassword != bufTCP) return DisplayString("[Fail] Invalid password", true);
        }
        else
            return DisplayString("[Fail] Receive password", true);
    }
    if (verbose) DisplayString("[Done] Client connection established");
    
    return 0; 
}

template <class TOrder, class TItem>
void NetServer<TOrder, TItem>::SendMsgOut()
{
    assert((sock_serv != sock_fail) && (sock_conn != sock_fail));
    const auto &msg = pBroker->BuildMsgOut();
    send(sock_conn, msg.c_str(), msg.size(), 0);
}

template <class TOrder, class TItem>
int NetServer<TOrder, TItem>::RecvMsgIn()
{
    assert((sock_serv != sock_fail) && (sock_conn != sock_fail));
    bufMsg[0] = '\0';
    while (true)
    {
        memset(bufTCP, 0, kTCPBufSize);
        if (recv(sock_conn, bufTCP, kTCPBufSize, 0) > 0)
        {
            std::size_t lencat = strlen(bufMsg) + strlen(bufTCP); 
            if (lencat >= kTCPBufSize) return DisplayString("TCP input buffer overflow", true);
            strcat(bufMsg, bufTCP);
            if (bufMsg[lencat - 1] == '\n')
                break;
        }
        else
        {
            if (verbose) DisplayString("Client disconnected");
            return -1;
        }
    }
    pBroker->ApplyMsgIn(bufMsg);
    return 0;
}

template <class TOrder, class TItem>
int NetServer<TOrder, TItem>::DisplayString(CSTR &disp, bool isError, int errno_)
{
    if (isError == false)
        std::cout << "NetServer: " << disp << std::endl;    
    else
    {
        std::cerr << "NetServer: " << disp;
        if (errno_ != 0) std::cerr << "\n" << strerror(errno_); 
        std::cerr << std::endl;
    }
    return (isError == false ? 0 : -1);
}

}

#endif /* SIBYL_SERVER_NETSERVER_H_ */