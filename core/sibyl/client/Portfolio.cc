
#include <iostream>
#include <cinttypes>
#include <cstring>

#include "Portfolio.h"

namespace sibyl
{

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
        
        if (i.Type() == SecType::ELW)
        {
            const auto &i = *dynamic_cast<ELW<Item>*>(code_pItem.second.get()); // reference as ELW<Item>
            state.isELW = true;
            state.iCP = (i.CallPut() == OptType::call) - (i.CallPut() == OptType::put);
            state.expiry = i.Expiry(); 
            state.kospi200 = ELW<Item>::kospi200;
            state.thr = i.thr;
        } else
            state.isELW = false;
        
        if (i.Type() == SecType::ETF)
        {
            const auto &i = *dynamic_cast<ETF<Item>*>(code_pItem.second.get()); // reference as ETF<Item>
            state.isETF = true;
            state.devNAV = i.devNAV;
        } else
            state.isETF = false;
        
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
        
        if (pcLine[0] == 'b')
        {
            // char cW : index of word (warning: overflows at 127)
            for (char *pcWord = strchr(pcLine, ' '), cW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), cW++)
            {
                while (*pcWord == ' ') pcWord++;
                if (cW == 1) sscanf(pcWord, "%d"      , &time);
                if (cW == 2) sscanf(pcWord, "%" SCNd64, &bal);
                if (cW == 3) sscanf(pcWord, "%" SCNd64, &sum.buy);
                if (cW == 4) sscanf(pcWord, "%" SCNd64, &sum.sell);
                if (cW == 5) sscanf(pcWord, "%" SCNd64, &sum.feetax);
            }
        }
        if (pcLine[0] == 's')
        {
            for (char *pcWord = strchr(pcLine, ' '), cW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), cW++)
            {
                while (*pcWord == ' ') pcWord++;
                if ((cW - 1) % 3 == 0) sscanf(pcWord, "%" SCNd64, &sum.tck_orig[(std::size_t)((cW - 1) / 3)].bal);
                if ((cW - 1) % 3 == 1) sscanf(pcWord, "%" SCNd64, &sum.tck_orig[(std::size_t)((cW - 1) / 3)].q  );
                if ((cW - 1) % 3 == 2) sscanf(pcWord, "%" SCNd64, &sum.tck_orig[(std::size_t)((cW - 1) / 3)].evt);
            }
        }
        if (pcLine[0] == 'k')
        {
            for (char *pcWord = strchr(pcLine, ' '), cW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), cW++)
            {
                while (*pcWord == ' ') pcWord++;
                if (cW == 1) sscanf(pcWord, "%f", &ELW<Item>::kospi200);
            }
        }
        if (pcLine[0] == 'd')
        {
            iM = std::end(items);
            for (char *pcWord = strchr(pcLine, ' '), cW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), cW++)
            {
                while (*pcWord == ' ') pcWord++;
                if (cW == 1)
                {
                    char *pcSpace = strchr(pcWord, ' ');
                    if (pcSpace != nullptr) *pcSpace = '\0';
                    STR c(pcWord);
                    if (pcSpace != nullptr) *pcSpace = ' ';
                    
                    iM = items.find(c);
                    if (iM == std::end(items))
                    {
                        auto it_bool = items.insert(std::make_pair(c, std::unique_ptr<Item>(new KOSPI<Item>)));
                        verify(it_bool.second == true); // assure successful insertion
                        iM = it_bool.first;
                    }
                }
                auto &i = *(iM->second); // reference to Item
                if  (cW ==  2)               sscanf(pcWord, "%f"      , &i.pr);
                if  (cW ==  3)               sscanf(pcWord, "%" SCNd64, &i.qr);
                if ((cW >=  4) && (cW < 24)) sscanf(pcWord, "%d"      , &i.tbr[(std::size_t)(cW -  4)].p);
                if ((cW >= 24) && (cW < 44)) sscanf(pcWord, "%d"      , &i.tbr[(std::size_t)(cW - 24)].q);
            }
        }
        if (pcLine[0] == 'e')
        {
            if (iM->second->Type() == SecType::KOSPI) // on first run: reallocate as ELW
            {
                auto &ptr = iM->second;
                KOSPI<Item> temp = *dynamic_cast<KOSPI<Item>*>(ptr.get()); // store copy
                ptr.reset(new ELW<Item>);
                ptr->pr   = temp.pr;
                ptr->qr   = temp.qr;
                ptr->tbr  = temp.tbr;
            }
            auto &i = *dynamic_cast<ELW<Item>*>(iM->second.get()); // reference as ELW<Item>
            
            OptType optType(OptType::null);
            int expiry = -1;
            int iCP = 0;
            for (char *pcWord = strchr(pcLine, ' '), cW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), cW++)
            {
                while (*pcWord == ' ') pcWord++;
                if  (cW ==  2)               sscanf(pcWord, "%d", &iCP);
                if  (cW ==  3)               sscanf(pcWord, "%d", &expiry);
                if ((cW >=  4) && (cW < 12)) sscanf(pcWord, "%f", &i.thr[(std::size_t)(cW - 4)]);
            }
            if (iCP == +1) optType = OptType::call;
            if (iCP == -1) optType = OptType::put;
            i.SetInfo(optType, expiry);
        }
        if (pcLine[0] == 'n')
        {
            if (iM->second->Type() == SecType::KOSPI) // on first run: reallocate as ETF
            {
                auto &ptr = iM->second;
                KOSPI<Item> temp = *dynamic_cast<KOSPI<Item>*>(ptr.get()); // store copy
                ptr.reset(new ETF<Item>);
                ptr->pr   = temp.pr;
                ptr->qr   = temp.qr;
                ptr->tbr  = temp.tbr;
            }
            auto &i = *dynamic_cast<ETF<Item>*>(iM->second.get()); // reference as ETF<Item>
            
            for (char *pcWord = strchr(pcLine, ' '), cW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), cW++)
            {
                while (*pcWord == ' ') pcWord++;
                if  (cW ==  2)               sscanf(pcWord, "%f", &i.devNAV);
            }
        }
        if (pcLine[0] == 'o')
        {
            auto &i = *(iM->second); // reference to Item
            i.ord.clear();
            Order o;
            for (char *pcWord = strchr(pcLine, ' '), cW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), cW++)
            {
                while (*pcWord == ' ') pcWord++;
                if  (cW == 2)                 sscanf(pcWord, "%d", &i.cnt);
                if ((cW >= 3) &&  (cW & 0x1)) sscanf(pcWord, "%d", &o.p);
                if ((cW >= 3) && !(cW & 0x1))
                {
                    sscanf(pcWord, "%d", &o.q);
                    verify(o.q != 0);
                    if      (o.q > 0)
                        o.type = OrdType::buy;
                    else if (o.q < 0) {
                        o.type = OrdType::sell;
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
            for (std::ptrdiff_t idx = 0; idx < idx::szTb; idx++)
            {
                sprintf(bufLine, "[%s%2d]\t%10d\t%10d\n", (idx <= idx::ps1 ? "s" : "b"), (idx <= idx::ps1 ? (int)(idx::ps1 - idx + 1) : (int)(idx - idx::pb1 + 1)), i.tbr[(std::size_t)idx].p, i.tbr[(std::size_t)idx].q);
                logVecOut << bufLine;
            }
        }
    }
    
    if (time >= kTimeBounds::end) return -1;
    return 0;
}

void Portfolio::WriteState()
{
    if (pathState.empty() == true) return;
    
    STR filename(pathState);
    if (time <= kTimeBounds::init)
        filename.append("client_ini.log");
    else if (time >= kTimeBounds::end - 60)
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
        fprintf(pf, "   bal  u  %12" PRId64 "\n" , se.balU      );
        fprintf(pf, "   bal b o %12" PRId64 "\n" , se.balBO     );
        fprintf(pf, "   evl cnt %12" PRId64 "\n" , se.evalCnt   );
        fprintf(pf, "   evl s o %12" PRId64 "\n" , se.evalSO    );
        fprintf(pf, "   evl tot %12" PRId64 " (r%+.2f%%) (s%+.2f%%)\n", se.evalTot, ((double)se.evalTot / balRef - 1.0) * 100.0, ((double)se.evalTot / balInit - 1.0) * 100.0);
        fprintf(pf, "   sum  b  %12" PRId64 "\n" , sum.buy   );
        fprintf(pf, "   sum  s  %12" PRId64 "\n" , sum.sell  );
        fprintf(pf, "   sum f+t %12" PRId64 "\n" , sum.feetax);

        fputs("\nsum  [t_o]          bal    quant      evt\n", pf);
        for (std::size_t idx = 0; idx < idx::szTb; idx++)
        {
            fprintf(pf, "     [%s%2d] ", ((int)idx <= idx::ps1 ? "s" : "b"), (idx <= idx::ps1 ? (int)idx::ps1 - (int)idx + 1 : (int)idx - (int)idx::pb1 + 1));
            fprintf(pf, "%12" PRId64 " %8" PRId64 " %8" PRId64 "\n", sum.tck_orig[idx].bal, sum.tck_orig[idx].q, sum.tck_orig[idx].evt);
            if (idx == idx::ps1)
            {
                fprintf(pf, "     [%s%2d] ", "s", 0);
                fprintf(pf, "%12" PRId64 " %8" PRId64 " %8" PRId64 "\n", sum.tck_orig[idxTckOrigS0].bal, sum.tck_orig[idxTckOrigS0].q, sum.tck_orig[idxTckOrigS0].evt);                        
                fprintf(pf, "     [%s%2d] ", "b", 0);
                fprintf(pf, "%12" PRId64 " %8" PRId64 " %8" PRId64 "\n", sum.tck_orig[idxTckOrigB0].bal, sum.tck_orig[idxTckOrigB0].q, sum.tck_orig[idxTckOrigB0].evt);
            }
        }

        fputs("\ncnt\n", pf);
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
                    if (o.type == type)
                    {
                        int tck = i.P2Tck(o.p, o.type); // 0-based tick
                        if (tck == idx::tckN) tck = 98;     // display as 99 if not found
                        fprintf(pf, "[%s%2d] {%s} %8d (%6d)", (type == OrdType::buy ? "b" : "s"), tck + 1, code_pItem.first.c_str(), o.p, o.q);
                        if (nItemPerLine == ++iCnt)
                        {
                            fputs("\n", pf);
                            iCnt = 0;
                        }
                        else
                            fputs(itemSpacer, pf);
                    }
                }
            }
            if (iCnt != 0)
                fputs("\n", pf);
        };
        
        fprintf(pf, "\nord  [t_c] (%4d)\n", nOrdTot);
        ListOrder(OrdType::buy) ; fputs("\n", pf);
        ListOrder(OrdType::sell); fputs("\n", pf);
        
        fclose (pf);
    }
    else
        std::cerr << "Portfolio: state path not accessible" << std::endl;
}

}
