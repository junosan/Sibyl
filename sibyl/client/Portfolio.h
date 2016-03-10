#ifndef SIBYL_CLIENT_PORTFOLIO_H_
#define SIBYL_CLIENT_PORTFOLIO_H_

#include <iostream>
#include <cinttypes>
#include <cstring>
#include <fstream>

#include "../Security.h"
#include "../Security_KOSPI.h"
#include "../Security_ELW.h"
#include "../Catalog.h"
#include "ItemState.h"

namespace sibyl
{

class Order : public PQ
{
public:
    OrdType type;
    Order() : type(kOrdNull) {}
};

class Item : public Security<Order> // abstract class; derive as stock, ELW, ETF, option, etc.
{
public:
    virtual ~Item() {}
};

class Portfolio : public Catalog<Item>
{
public:
    // to be used by rnn client
    const std::vector<ItemState>& GetStateVec();
    
    // to be called by Trader
    void SetStateLogPaths(CSTR &state, CSTR &log);
    int  ApplyMsgIn      (char *msg); // this destroys msg during parsing; returns non-0 to signal termination
private:
    std::vector<ItemState> vecState;
    
    STR pathState;
    STR pathLog;
    
    void WriteState(); // writes only if state path was set
    
    std::ofstream logMsgIn;  // full state msg
    std::ofstream logVecOut; // t, pr, qr, tbr
    
    char bufLine[1 << 10];
};

const std::vector<ItemState>& Portfolio::GetStateVec()
{
    if (items.size() != vecState.size()) vecState.resize(items.size());

    std::size_t codeIdx = 0;
    for (const auto &code_pItem : items)
    {
        const auto &i     = *code_pItem.second; 
              auto &state = vecState[codeIdx];
        state.code = code_pItem.first; 
        state.time = time;
        state.pr   = i.pr;
        state.qr   = i.qr;
        state.tbr  = i.tbr;
        codeIdx++;
    }
    return vecState;
}

void Portfolio::SetStateLogPaths(CSTR &state, CSTR &log)
{
    pathState = state;
    if ((pathState.empty() == false) && ('/' != pathState[pathState.size() - 1])) pathState.append("/");
    pathLog   = log;
    if ((pathLog.empty()   == false) && ('/' != pathLog  [pathLog.size()   - 1])) pathLog.append("/");
    
    if (pathLog.empty() == false)
    {
        logMsgIn.open (pathLog + "msg_in.log" , std::ofstream::trunc);
        logVecOut.open(pathLog + "vec_out.log", std::ofstream::trunc);
        if ((logMsgIn.is_open() == false) || (logVecOut.is_open() == false))
            std::cerr << "Portfolio: log path not accessible" << std::endl;
    }
}

int Portfolio::ApplyMsgIn(char *msg) // Parse message and update entries
{
    assert((timeBounds.ref  > TimeBounds::null) &&
           (timeBounds.init > TimeBounds::null) &&
           (timeBounds.stop > TimeBounds::null) &&
           (timeBounds.end  > TimeBounds::null) ); // must call SetTimeBoundaries(int, int, int, int) first
    
    if (logMsgIn.is_open() == true) // log raw input from server
    {
        int timeMsg;
        sscanf(msg + 6, "%d", &timeMsg); // msg starts with "/*\nb [time]"
        logMsgIn << "[t=" << timeMsg << "]\n"; 
        logMsgIn.write(msg, (std::streamsize)strlen(msg));
    }
    
    auto iM = std::end(items);
    for (char *pcLine = strtok(msg, "\n"); pcLine != nullptr; pcLine = strtok(nullptr, "\n"))
    {
        char *pc = strpbrk(pcLine, "\r\n");
        if (pc != NULL) *pc = '\0';
        if (strlen(pcLine) == 0) continue;
        
        int iW = 0; // index of word
        if (pcLine[0] == 'b')
        {
            for (char *pcWord = strchr(pcLine, ' '), iW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), iW++)
            {
                while (*pcWord == ' ') pcWord++;
                if (iW == 1) sscanf(pcWord, "%d"      , &time);
                if (iW == 2) sscanf(pcWord, "%" SCNd64, &bal);
                if (iW == 3) sscanf(pcWord, "%" SCNd64, &sum.buy);
                if (iW == 4) sscanf(pcWord, "%" SCNd64, &sum.sell);
                if (iW == 5) sscanf(pcWord, "%" SCNd64, &sum.feetax);
            }
        }
        if (pcLine[0] == 's')
        {
            for (char *pcWord = strchr(pcLine, ' '), iW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), iW++)
            {
                while (*pcWord == ' ') pcWord++;
                if ((iW - 1) % 3 == 0) sscanf(pcWord, "%" SCNd64, &sum.tck_orig[(std::size_t)((iW - 1) / 3)].bal);
                if ((iW - 1) % 3 == 1) sscanf(pcWord, "%" SCNd64, &sum.tck_orig[(std::size_t)((iW - 1) / 3)].q  );
                if ((iW - 1) % 3 == 2) sscanf(pcWord, "%" SCNd64, &sum.tck_orig[(std::size_t)((iW - 1) / 3)].evt);
            }
        }
        if (pcLine[0] == 'k')
        {
            for (char *pcWord = strchr(pcLine, ' '), iW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), iW++)
            {
                while (*pcWord == ' ') pcWord++;
                if (iW == 1) sscanf(pcWord, "%f", &ELW<Item>::kospi200);
            }
        }
        if (pcLine[0] == 'd')
        {
            iM = std::end(items);
            for (char *pcWord = strchr(pcLine, ' '), iW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), iW++)
            {
                while (*pcWord == ' ') pcWord++;
                if (iW == 1)
                {
                    char *pcSpace = strchr(pcWord, ' ');
                    if (pcSpace != nullptr) *pcSpace = '\0';
                    STR c(pcWord);
                    if (pcSpace != nullptr) *pcSpace = ' ';
                    
                    iM = items.find(c);
                    if (iM == std::end(items))
                    {
                        auto it_bool = items.insert(std::make_pair(c, std::unique_ptr<Item>(new KOSPI<Item>)));
                        assert(it_bool.second == true); // assure successful insertion
                        iM = it_bool.first;
                    }
                }
                auto &i = *(iM->second); // reference to Item
                if  (iW ==  2)               sscanf(pcWord, "%f"      , &i.pr);
                if  (iW ==  3)               sscanf(pcWord, "%" SCNd64, &i.qr);
                if ((iW >=  4) && (iW < 24)) sscanf(pcWord, "%d"      , &i.tbr[(std::size_t)(iW -  4)].p);
                if ((iW >= 24) && (iW < 44)) sscanf(pcWord, "%d"      , &i.tbr[(std::size_t)(iW - 24)].q);
            }
        }
        if (pcLine[0] == 'e')
        {
            if (iM->second->Type() == kSecKOSPI) // on first run: reallocate as ELW
            {
                auto &ptr = iM->second;
                KOSPI<Item> temp = *dynamic_cast<KOSPI<Item>*>(ptr.get());
                ptr.reset(new ELW<Item>);
                ptr->pr   = temp.pr;
                ptr->qr   = temp.qr;
                ptr->tbr  = temp.tbr;
            }
            auto &i = *dynamic_cast<ELW<Item>*>(iM->second.get()); // reference to ELW<Item>
            
            OptType optType = kOptNull;
            int expiry = -1;
            int iCP = 0;
            for (char *pcWord = strchr(pcLine, ' '), iW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), iW++)
            {
                while (*pcWord == ' ') pcWord++;
                if  (iW ==  2)               sscanf(pcWord, "%d", &iCP);
                if  (iW ==  3)               sscanf(pcWord, "%d", &expiry);
                if ((iW >=  4) && (iW < 12)) sscanf(pcWord, "%f", &i.thr[(std::size_t)(iW - 4)]);
            }
            if (iCP == +1) optType = kOptCall;
            if (iCP == -1) optType = kOptPut;
            i.SetInfo(optType, expiry);
        }
        if (pcLine[0] == 'o')
        {
            auto &i = *(iM->second); // reference to Item
            i.ord.clear();
            Order o;
            for (char *pcWord = strchr(pcLine, ' '), iW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), iW++)
            {
                while (*pcWord == ' ') pcWord++;
                if  (iW == 2)                 sscanf(pcWord, "%d", &i.cnt);
                if ((iW >= 3) &&  (iW & 0x1)) sscanf(pcWord, "%d", &o.p);
                if ((iW >= 3) && !(iW & 0x1))
                {
                    sscanf(pcWord, "%d", &o.q);
                    assert(o.q != 0);
                    if      (o.q > 0)
                        o.type = kOrdBuy;
                    else if (o.q < 0) {
                        o.type = kOrdSell;
                        o.q = -o.q;
                    }
                    i.ord.insert(std::make_pair(o.p, o));
                }
            }
        }
    }
    
    UpdateRefInitBal();
    
    if (pathState.empty() == false) {
        WriteState();
    }

    if (logVecOut.is_open() == true) // log t, pr, qr, tbr values
    {
        logVecOut << "[t=" << time << "]\n";
        for (const auto &code_pItem : items)
        {
            Item &i = *code_pItem.second;
            sprintf(bufLine, "{%s}\n"               , code_pItem.first.c_str()); logVecOut << bufLine;
            sprintf(bufLine, "t\t%10d\n"            , time);                     logVecOut << bufLine;
            sprintf(bufLine, "pr\t%.4e\n"           , i.pr);                     logVecOut << bufLine;
            sprintf(bufLine, "qr\t%10" PRId64 "\n"  , i.qr);                     logVecOut << bufLine;
            sprintf(bufLine, "     \ttbpr\t\ttbqr\n");                           logVecOut << bufLine;
            for (std::ptrdiff_t idx = 0; idx < szTb; idx++)
            {
                sprintf(bufLine, "[%s%2d]\t%10d\t%10d\n", (idx <= idxPs1 ? "s" : "b"), (idx <= idxPs1 ? (int)(idxPs1 - idx + 1) : (int)(idx - idxPb1 + 1)), i.tbr[(std::size_t)idx].p, i.tbr[(std::size_t)idx].q);
                logVecOut << bufLine;
            }
        }
    }
    
    if (time >= timeBounds.end) return -1;
    return 0;
}

void Portfolio::WriteState()
{
    if (pathState.empty() == true) return;
    
    STR filename(pathState);
    if (time <= timeBounds.init)
        filename.append("client_ini.log");
    else if (time >= timeBounds.end - 60)
        filename.append("client_fin.log");
    else
        filename.append("client_cur.log");
    
    const int  nItemPerLine = 5;
    const char itemSpacer[] = "    ";
    
    FILE* pf = fopen(filename.c_str(), "w");
    if (pf != nullptr)
    {
        SEval se = Evaluate();
        
        fprintf(pf, "t = %5d sec\n", time);
        fprintf(pf, "   Bal  u  %12" PRId64 "\n" , se.balU      );
        fprintf(pf, "   Bal b o %12" PRId64 "\n" , se.balBO     );
        fprintf(pf, "   Evl cnt %12" PRId64 "\n" , se.evalCnt   );
        fprintf(pf, "   Evl s o %12" PRId64 "\n" , se.evalSO    );
        fprintf(pf, "   Evl tot %12" PRId64 " (r%+.2f%%) (s%+.2f%%)\n", se.evalTot, ((double)se.evalTot / balRef - 1.0) * 100.0, ((double)se.evalTot / balInit - 1.0) * 100.0);
        fprintf(pf, "   Sum  b  %12" PRId64 "\n" , sum.buy   );
        fprintf(pf, "   Sum  s  %12" PRId64 "\n" , sum.sell  );
        fprintf(pf, "   Sum f+t %12" PRId64 "\n" , sum.feetax);

        fputs("\nStat [tck_orig]     bal    quant      evt\n", pf);
        for (std::size_t idx = 0; idx < szTb; idx++)
        {
            fprintf(pf, "     [%s%2d] ", ((int)idx <= idxPs1 ? "s" : "b"), (idx <= idxPs1 ? (int)idxPs1 - (int)idx + 1 : (int)idx - (int)idxPb1 + 1));
            fprintf(pf, "%12" PRId64 " %8" PRId64 " %8" PRId64 "\n", sum.tck_orig[idx].bal, sum.tck_orig[idx].q, sum.tck_orig[idx].evt);
            if (idx == idxPs1)
            {
                fprintf(pf, "     [%s%2d] ", "s", 0);
                fprintf(pf, "%12" PRId64 " %8" PRId64 " %8" PRId64 "\n", sum.tck_orig[idxTckOrigS0].bal, sum.tck_orig[idxTckOrigS0].q, sum.tck_orig[idxTckOrigS0].evt);                        
                fprintf(pf, "     [%s%2d] ", "b", 0);
                fprintf(pf, "%12" PRId64 " %8" PRId64 " %8" PRId64 "\n", sum.tck_orig[idxTckOrigB0].bal, sum.tck_orig[idxTckOrigB0].q, sum.tck_orig[idxTckOrigB0].evt);
            }
        }

        fputs("\nCnt\n", pf);
        int iCnt = 0;    // number of items in current line
        int nOrdTot = 0; // total number of orders
        for (const auto &code_pItem : items)
        {
            const auto &i = *code_pItem.second;
            if (i.cnt > 0)
            {
                fprintf(pf, "      {%s} %8d (%6d)", code_pItem.first.c_str(), i.Ps0(), i.cnt);
                if (nItemPerLine == ++iCnt)
                {
                    fputs("\n", pf);
                    iCnt = 0;
                }
                else
                    fputs(itemSpacer, pf);
            }
            nOrdTot += i.ord.size();
        }
        if (iCnt != 0) fputs("\n", pf);		                            
        
        auto ListOrder = [&](const OrdType &type) {
            int iCnt = 0;
            for (const auto &code_pItem : items)
            {
                const auto &i = *code_pItem.second;
                for (const auto &price_Order : i.ord)
                {
                    const auto &o = price_Order.second;
                    int tck = i.P2Tck(o.p, o.type); // 0-based tick
                    if (tck == szTck) tck = 98;     // display as 99 if not found
                    fprintf(pf, "[%s%2d] {%s} %8d (%6d)", (type == kOrdBuy ? "b" : "s"), tck + 1, code_pItem.first.c_str(), o.p, o.q);
                    if (nItemPerLine == ++iCnt)
                    {
                        fputs("\n", pf);
                        iCnt = 0;
                    }
                    else
                        fputs(itemSpacer, pf);
                }
            }
            if (iCnt != 0)
                fputs("\n", pf);
        };
        
        fprintf(pf, "\nOrd [tck_cur] (%4d)\n", nOrdTot);
        ListOrder(kOrdBuy) ; fputs("\n", pf);
        ListOrder(kOrdSell); fputs("\n", pf);
        
        fclose (pf);
    }
    else
        std::cerr << "Portfolio: state path not accessible" << std::endl;
}

}

#endif  /* SIBYL_CLIENT_PORTFOLIO_H_ */