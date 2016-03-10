#ifndef SIBYL_SERVER_BROKER_H_
#define SIBYL_SERVER_BROKER_H_

#include <mutex>
#include <iostream>

#include "../Participant.h"
#include "OrderBook.h"

namespace sibyl
{

template <class TOrder, class TItem>
class Broker : public Participant
{
public:
    OrderBook<TOrder, TItem> orderbook;
    
    // interface for wait/wake mechanism 
    bool IsWoken();
    void Wake   ();
    
    virtual // Simulation: process data until next tick; Kiwoom: wait until event thread wakes it up
    int   AdvanceTick() = 0; // returns non-0 to signal exit
    virtual const // implement depending on addMyOrd
    CSTR& BuildMsgOut() = 0;
    void  ApplyMsgIn (char *msg);
    
    Broker() : wake_bool(false) {}
protected:
    bool                    wake_bool;  // don't use this without locking wake_mutex
    std::mutex              wake_mutex;
    std::condition_variable wake_cv;

    const
    std::vector<UnnamedReq<TItem>>& ParseMsgIn(char *msg); // this destroys msg during parsing
    void ExecuteUnnamedReqs(const std::vector<UnnamedReq<TItem>>& ureq);
    virtual
    int  ExecuteNamedReq   (NamedReq<TOrder, TItem> req) = 0; // returns non-0 to signal exit (req count limit)

    void InitializeMembers();
private:    
    std::vector<UnnamedReq<TItem>> ureq;
};

template <class TOrder, class TItem>
bool Broker<TOrder, TItem>::IsWoken()
{
    std::lock_guard<std::mutex> lock(wake_mutex);
    return wake_bool;
}

template <class TOrder, class TItem>
void Broker<TOrder, TItem>::Wake()
{
    {
        std::lock_guard<std::mutex> lock(wake_mutex);
        wake_bool = true;
    }
    wake_cv.notify_one(); // don't notify with the mutex locked
}

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
                if (word == "b" ) req.type = kReq_b;
                if (word == "s" ) req.type = kReq_s;
                if (word == "cb") req.type = kReq_cb;
                if (word == "cs") req.type = kReq_cs;
                if (word == "mb") req.type = kReq_mb;
                if (word == "ms") req.type = kReq_ms;
                if (word == "ca") req.type = kReq_ca;
                if (word == "sa") req.type = kReq_sa;
                if ((word == "ca") || (word == "sa")) fail = false; // prepare to exit
                if (req.type == kReqNull) break; // command not found
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
                if ((req.type != kReq_mb) && (req.type != kReq_ms)) fail = false; // prepare to exit
            }
            if (iW == 4) // modprice (mb/ms only)
            {
                if ((req.type != kReq_mb) && (req.type != kReq_ms)) { fail = true; break; } // too many words for a b/s/cb/cs command
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
        
        if (fail == false) ureq.push_back(req);
        else
        {
            std::cerr << "Invalid order: " << pcLine << std::endl;
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
        bool interruptible = (req.type != kReq_ca) && (req.type != kReq_sa); 
        if ((interruptible == true) && (IsWoken() == true)) break;
        const auto &nreq = orderbook.AllotReq(req);
        for (const auto &req : nreq)
        {
            if ((interruptible == true) && (IsWoken() == true)) break;
            int ret = ExecuteNamedReq(req);
            if ((interruptible == true) && (ret != 0)) break; 
        }
    }
}

template <class TOrder, class TItem>
void Broker<TOrder, TItem>::InitializeMembers()
{
    orderbook.SetTimeBounds(timeBounds);
}

}

#endif /* SIBYL_SERVER_BROKER_H_ */