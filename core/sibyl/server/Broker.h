#ifndef SIBYL_SERVER_BROKER_H_
#define SIBYL_SERVER_BROKER_H_

#include <cstring>
#include <iostream>
#include <atomic>

#include "OrderBook.h"
#include "../util/DispPrefix.h"
#include "../ReqType.h"

namespace sibyl
{

template <class TOrder, class TItem>
class Broker
{
public:
    OrderBook<TOrder, TItem> orderbook;
    
    void SetVerbose(bool verbose_) { verbose = verbose_; orderbook.SetVerbose(verbose); }
    DispPrefix dispPrefix;
    
    // skip & interrupt mechanism 
    bool IsSkipping   () { return skipTick;     }
    bool IsInterrupted() { return ab_interrupt; }
    void InterruptExec() { ab_interrupt = true; }
    
    virtual // Simulation: process data until next tick; Kiwoom: wait until event thread wakes it up
    int   AdvanceTick() = 0; // returns non-0 to signal exit
    virtual const // implement depending on addMyOrd
    CSTR& BuildMsgOut() = 0;
    void  ApplyMsgIn (char *msg);
    
    Broker() : verbose(false),
               skipTick(false),
               ab_interrupt(false) {}
protected:
    bool verbose;
    
    // skip & interrupt mechanism
    bool skipTick;
    void ResetInterrupt() { ab_interrupt = false; }

    const
    std::vector<UnnamedReq<TItem>>& ParseMsgIn(char *msg); // this destroys msg during parsing
    void ExecuteUnnamedReqs(const std::vector<UnnamedReq<TItem>>& ureq);
    virtual
    int ExecuteNamedReq(NamedReq<TOrder, TItem> req) = 0; // returns non-0 to signal exit (req count limit)

private:
    std::atomic_bool ab_interrupt;
    std::vector<UnnamedReq<TItem>> ureq;
};

template <class TOrder, class TItem>
void Broker<TOrder, TItem>::ApplyMsgIn(char *msg)
{
    orderbook.RemoveEmptyOrders();
    ExecuteUnnamedReqs(ParseMsgIn(msg));
}

template <class TOrder, class TItem>
const std::vector<UnnamedReq<TItem>>& Broker<TOrder, TItem>::ParseMsgIn(char *msg)
{
    ureq.clear();

    for (char *pcLine = strtok(msg, "\n"); pcLine != NULL; pcLine = strtok(NULL, "\n"))
    {
        char *pc = strpbrk(pcLine, "\r\n");
        if (pc != NULL) *pc = '\0';
        if (strlen(pcLine) == 0) continue;            
        
        UnnamedReq<TItem> req;
        bool fail = true;
        int iW = 0;
        for (char *pcWord = pcLine; pcWord != NULL; pcWord = strchr (pcWord, ' '), iW++)
        {                
            while (*pcWord == ' ') pcWord++; // prevent empty word
            char *pcSpace = strchr(pcWord, ' ');
            if (pcSpace != NULL) *pcSpace = '\0';
            STR word(pcWord);
            if (pcSpace != NULL) *pcSpace = ' ';
            
            if (iW == 0) // command
            {
                if (word == "b" ) req.type = ReqType::b;
                if (word == "s" ) req.type = ReqType::s;
                if (word == "cb") req.type = ReqType::cb;
                if (word == "cs") req.type = ReqType::cs;
                if (word == "mb") req.type = ReqType::mb;
                if (word == "ms") req.type = ReqType::ms;
                if (word == "ca") req.type = ReqType::ca;
                if (word == "sa") req.type = ReqType::sa;
                if ((word == "ca") || (word == "sa")) fail = false; // prepare to exit
                if (req.type == ReqType::null) break; // command not found
            }
            if (iW == 1) // code
            {
                if ((word == "ca") || (word == "sa")) { fail = true; break; } // too many words for a ca/sa command
                req.iItems = orderbook.items.find(word);
                if (req.iItems == std::end(orderbook.items)) break; // code not found
            }
            if (iW == 2) // price
            {
                if (false == std::all_of(std::begin(word), std::end(word), (int (*)(int))std::isdigit)) break; // not non-negative number
                req.p = std::stoi(word);
                if (req.p <= 0) break; // 0 price not allowed
            }
            if (iW == 3) // quantity
            {
                if (false == std::all_of(std::begin(word), std::end(word), (int (*)(int))std::isdigit)) break; // not non-negative number
                req.q = std::stoi(word);
                if ((req.type != ReqType::mb) && (req.type != ReqType::ms)) fail = false; // prepare to exit
            }
            if (iW == 4) // modprice (mb/ms only)
            {
                if ((req.type != ReqType::mb) && (req.type != ReqType::ms)) { fail = true; break; } // too many words for a b/s/cb/cs command
                if (false == std::all_of(std::begin(word), std::end(word), (int (*)(int))std::isdigit)) break; // not non-negative number
                req.mp = std::stoi(word);
                if (req.mp <= 0) break; // 0 price not allowed
                fail = false; // prepare to exit
            }
            if (iW == 5) // too many words for any command
            {
                fail = true;
                break;
            }
        }
        
        if (fail == false)
        {
            ureq.push_back(req);
            if (verbose == true) std::cout << dispPrefix << "ReqIn: " << pcLine << std::endl;
        }
        else
        {
            std::cerr << dispPrefix << "Invalid req: " << pcLine << std::endl;
            continue;
        }
    }
    
    return ureq;
}

template <class TOrder, class TItem>
void Broker<TOrder, TItem>::ExecuteUnnamedReqs(const std::vector<UnnamedReq<TItem>>& ureq)
{
    for (const auto &req : ureq)
    {
        bool interruptible = (req.type != ReqType::ca && req.type != ReqType::sa); 
        if (interruptible == true && IsInterrupted() == true) break;
        const auto &nreq = orderbook.AllotReq(req);
        for (const auto &req : nreq)
        {
            if (interruptible == true && IsInterrupted() == true) break;
            int ret = ExecuteNamedReq(req);
            if (interruptible == true && ret != 0) break; 
        }
    }
}

}

#endif /* SIBYL_SERVER_BROKER_H_ */