#ifndef SIBYL_PORTFOLIO_H_
#define SIBYL_PORTFOLIO_H_

#include <cinttypes>
#include <cstring>
#include <fstream>

#include "Security.h"
#include "Catalog.h"

namespace sibyl
{

class Order : public PQ
{
public:
    OrdType type;
    Order() : type(kOrdNull) {}
};

class Item : public Security<std::vector<Order>> // abstract class; derive as stock, ELW, ETF, option, etc.
{
public:
    struct {
        FLOAT s;
        FLOAT b;
    } G0;
    struct Gn {
        FLOAT s;
        FLOAT b;
        FLOAT cs;
        FLOAT cb;
    };
    std::array<Gn, szTck> G;
    Item() : G0{}, G{} {}
    virtual ~Item() {}
};

class Portfolio : public Catalog<Item>
{
public:
    void SetParams        (double timeConst_, double rhoWeight_, double rhoInit_) {
        assert((timeConst_ > 0.0) && (rhoWeight_ >= 0.0) && (rhoInit_ >= 0.0));
        timeConst = timeConst_; rhoWeight = rhoWeight_; rho = rhoInit_;
    }
    void SetStateLogPaths (const std::string &state, const std::string &log);
    
    int  ReadMsgIn  (char *msg); // this destroys msg during parsing; returns non-0 to signal termination
    void BuildMsgOut(std::string &msg);
    
    Portfolio() : timeConst(0.0), // for initialization check
                  rhoWeight(0.0), rho(0.0) {}
private:
    double timeConst;
    double rhoWeight;
    double rho;
    
    std::string pathState;
    std::string pathLog;
    
    void WriteState  (); // writes only if state path was set
    void WritePosGCnt(); // writes only if state path was set
    
    std::ofstream logMsgIn;  // full state msg
    std::ofstream logMsgOut; // req msg
    std::ofstream logVecIn;  // G
    std::ofstream logVecOut; // t, pr, qr, tbr
    
    // for binary log of G values
    std::map<std::string, std::unique_ptr<FILE, int(*)(FILE*)>> mfRef;
    
    int nMsgCurTick;
    char bufLine[1 << 10];
    bool CatReq(std::string &msg, const char *pcMsgType, const std::string &code, INT price, INT quant, INT modprice = 0)
    {
        if (modprice > 0) sprintf(bufLine, "%s %s %d %d %d\n", pcMsgType, code.c_str(), price, quant, modprice);
        else              sprintf(bufLine, "%s %s %d %d\n"   , pcMsgType, code.c_str(), price, quant);
        msg.append(bufLine);
        if (kReqNPerTick == ++nMsgCurTick)
            return true;
        return false;        
    }
};

class GTck {
public:
    FLOAT G;
    std::map<std::string, std::unique_ptr<Item>>::iterator iM;
    int tick;           // 0-based index of price (-1: Gs0/Gb0, and so on)
    GTck() : G(0.0f), tick(0) {}
};

class GReq {
public:
    FLOAT G;            // GTck * price * quant * (1 +- fee/tax)
    ReqType type;    
    std::map<std::string, std::unique_ptr<Item>>::iterator iM;
    INT price;          // actual price
    INT quant;          // actual quantity
    INT modprice;       // actual mod price
    GReq() : G(0.0f), type(kReqNull), price(0), quant(0), modprice(0) {}
};

void Portfolio::SetStateLogPaths(const std::string &state, const std::string &log)
{
    pathState = state;
    if ((pathState.empty() == false) && ('/' != pathState[pathState.size() - 1])) pathState.append("/");
    pathLog   = log;
    if ((pathLog.empty()   == false) && ('/' != pathLog  [pathLog.size()   - 1])) pathLog.append("/");
    
    if (pathLog.empty() == false)
    {
        logMsgIn.open (pathLog + "msg_in.log" , std::ofstream::trunc);
        logMsgOut.open(pathLog + "msg_out.log", std::ofstream::trunc);
        logVecIn.open (pathLog + "vec_in.log" , std::ofstream::trunc);
        logVecOut.open(pathLog + "vec_out.log", std::ofstream::trunc);
        if ((logMsgIn.is_open() == false) || (logMsgOut.is_open() == false) || (logVecIn.is_open() == false) || (logVecOut.is_open() == false))
            std::cerr << "Portfolio: log path not accessible" << std::endl;
    }
}

int Portfolio::ReadMsgIn(char *msg)
// Parse message and update entries
{
    assert((timeRef  > Catalog<Item>::timeDefault) &&
           (timeInit > Catalog<Item>::timeDefault) &&
           (timeStop > Catalog<Item>::timeDefault) &&
           (timeEnd  > Catalog<Item>::timeDefault) ); // must call SetTimeBoundaries(int, int, int, int) first
    
    if (logMsgIn.is_open() == true) // log raw input from server
    {
        int timeMsg;
        sscanf(msg + 6, "%d", &timeMsg); // msg starts with "/*\nb [time]"
        logMsgIn << "[t = " << timeMsg << "]\n"; 
        logMsgIn.write(msg, (std::streamsize)strlen(msg));
    }
    
    auto iM = std::end(map);
    for (char *pcLine = strtok(msg, "\n"); pcLine != nullptr; pcLine = strtok(nullptr, "\n"))
    {
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
            iM = std::end(map);
            for (char *pcWord = strchr(pcLine, ' '), iW = 1; pcWord != nullptr; pcWord = strchr(pcWord, ' '), iW++)
            {
                while (*pcWord == ' ') pcWord++;
                if (iW == 1)
                {
                    char *pcSpace = strchr(pcWord, ' ');
                    if (pcSpace != nullptr) *pcSpace = '\0';
                    std::string c(pcWord);
                    if (pcSpace != nullptr) *pcSpace = ' ';
                    
                    iM = map.find(c);
                    if (iM == std::end(map))
                    {
                        auto it_bool = map.insert(std::make_pair(c, std::unique_ptr<Item>(new Stock<Item>)));
                        assert(it_bool.second == true); // assure successful insertion
                        iM = it_bool.first;
                        
                        if (pathLog.empty() == false)
                        {
                            std::string filename = pathLog + iM->first + ".ref";
                            FILE *pf = fopen(filename.c_str(), "wb");
                            assert(pf != nullptr); // kill, as fclose(nullptr) is a crash anyways
                            auto it_bool = mfRef.insert(std::make_pair(c, std::unique_ptr<FILE, int(*)(FILE*)>(pf, fclose)));
                            assert(it_bool.second == true);
                        }
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
            if (iM->second->Type() == kSecStock) // on first run: reallocate as ELW
            {
                auto &ptr = iM->second;
                Stock<Item> temp = *dynamic_cast<Stock<Item>*>(ptr.get());
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
                    i.ord.push_back(o);
                }
            }
        }
    }
    
    if (logVecOut.is_open() == true) // log t, pr, qr, tbr values
    {
        logVecOut << "[t = " << time << "]\n";
        for (const auto &code_pItem : map)
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
    
    UpdateRefInitBal();
    
    if (time >= timeEnd) return -1;
    return 0;
}

void Portfolio::BuildMsgOut(std::string &msg)
// Sell orders
// 1. For each code, define PGi, where i spans all (0~10th) prices with Gs > 0
// 2. EG=sum(PGi*PGi)/sum(PGi) (EG=0 if sum(PGi)==0)
// 3. Cancel all orders with Gc+EG>0
// 4. Place distributed sell orders with quantity proportional to PGi
// Buy orders
// 1. For all codes, define PGi, where i spans all (0~10th) prices of all codes with Gb > 0
// 2. EG=sum(PGi*PGi)/sum(PGi) (EG=0 if sum(PGi)==0)
// 3. Cancel all orders with Gc+EG>0
// 4. Set aside rho / sum(PGi) amount of total fund, where rho is a moving averaged EG; rho may have a weight
// 5. Distribute fund proportional to PGi, and place distributed buy orders of quantity fitting the distributed fund
//
// Put cancel and new order in buffers first and match cancel->new order pairs to replace with modify orders 
{
    assert(timeConst > 0.0);
    msg.clear();
    
    if (pathState.empty() == false) {
        WritePosGCnt();
        WriteState();
    }
    if ((logVecIn.is_open() == true) && // log G values
        (time >= timeInit) && (time < timeStop))
    {
        logVecIn << "[t = " << time << "]\n";
        for (const auto &code_pItem : map)
        {
            // text log
            Item &i = *code_pItem.second;
            sprintf(bufLine, "{%s}\n"                 , code_pItem.first.c_str()); logVecIn << bufLine;
            sprintf(bufLine, "Gs0\t%+.3e\n"           , i.G0.s);                   logVecIn << bufLine;
            sprintf(bufLine, "Gb0\t%+.3e\n"           , i.G0.b);                   logVecIn << bufLine;
            sprintf(bufLine, "     \tGs/b\t\tGcs/cb\n");                           logVecIn << bufLine;
            for (std::ptrdiff_t idx = 0; idx <= idxPs1; idx++) {
                sprintf(bufLine, "[s%2d]\t%+.3e\t%+.3e\n", (int)(1 + idxPs1 - idx), i.G[(std::size_t)(idxPs1 - idx)].s, i.G[(std::size_t)(idxPs1 - idx)].cs);
                logVecIn << bufLine;
            }
            for (std::ptrdiff_t idx = 0; idx < szTck; idx++) {
                sprintf(bufLine, "[b%2d]\t%+.3e\t%+.3e\n", (int)(1 + idx)         , i.G[(std::size_t)idx].b           , i.G[(std::size_t)idx].cb           );
                logVecIn << bufLine;
            }
            
            // binary log
            std::size_t szDim = 42;
            float data[szDim];
            auto imf = mfRef.find(code_pItem.first);
            assert(imf != std::end(mfRef));
            FILE *pf = &*(imf->second);
            data[0] = i.G0.s;
            data[1] = i.G0.b;
            
            std::ptrdiff_t tck = 0;
            for (const auto &gn : i.G)
            {
                data[ 2 + tck  ] = gn.s;
                data[12 + tck  ] = gn.b; 
                data[22 + tck  ] = gn.cs;
                data[32 + tck++] = gn.cb; 
            }
            fwrite(data, sizeof(float), szDim, pf);
        }
    }
    
    const int timeCancelAll  = 21000 + 600 + 60;
    
    // Stop buying anything between 14:30 and 14:50
    if ((time >= timeStop - 1200) && (time < timeStop))
    {
        for (const auto &code_pItem : map)
        {
            auto &i = *code_pItem.second;
            i.G0.b    = -100.0f;
            for (auto &gn : i.G)
            {
                gn.b  = -100.0f;
                gn.cb =  100.0f;
            }
        }
    }
    
    // Don't do anything between 14:50 and 15:01
    if ((time >= timeStop) && (time < timeCancelAll))
    {
        for (const auto &code_pItem : map)
        {
            auto &i = *code_pItem.second;
            i.G0.b    = -100.0f;
            i.G0.s    = -100.0f;
            for (auto &gn : i.G)
            {
                gn.b  = -100.0f;
                gn.cb =  100.0f;
                gn.s  = -100.0f;
                gn.cs = -100.0f;
            }
        }
    }
    
    // Cancel everything after 15:01
    if (time >= timeCancelAll)
    {
        for (const auto &code_pItem : map)
        {
            auto &i = *code_pItem.second;
            i.G0.b    = -100.0f;
            i.G0.s    = -100.0f;
            for (auto &gn : i.G)
            {
                gn.b  = -100.0f;
                gn.cb =  100.0f;
                gn.s  = -100.0f;
                gn.cs =  100.0f;
            }
        }        
    }
    
    // between 09:00:int and 15:01:00
    if (time >= timeInit)
    {
        // Annihilate self-contradictory inputs
        auto Annihilate = [](FLOAT &a, FLOAT &b) {
            if ((a > 0.0f) && (b > 0.0f)) {
                if (a > b) { a = a - b; b = 0.0f; }
                else       { b = b - a; a = 0.0f; }
            } 
        };
        for (const auto &code_pItem : map)
        {
            auto &i = *code_pItem.second;
            Annihilate(i.G0.b, i.G0.s);
            for (auto &gn : i.G)
            {
                Annihilate(gn.b, gn.cb);
                Annihilate(gn.s, gn.cs);
            }
        }
        
        nMsgCurTick = 0;
        std::vector<GReq> cReq; // Buf for cs, cb
        std::vector<GReq> oReq; // Buf for s, ms, b, mb 

        // Sell reqs
        for (auto iM = std::begin(map); iM != std::end(map); iM++)
        {
            const auto &code_pItem = *iM;
            const auto &i = *code_pItem.second;

            // Register positive Gs in intermediate buf and calculate EG 
            std::vector<GTck> vPosGs;
            GTck posGs;
            double sumPosGs  = 0.0;
            double sumPosGs2 = 0.0;
            bool doG0s = true;
            for (auto iG = std::begin(i.G); iG != std::end(i.G);) {
                if ((posGs.G   = (doG0s == true ? i.G0.s : iG->s)) > 0.0f)
                {
                    posGs.tick = (doG0s == true ? -1 : (int)(iG - std::begin(i.G)));
                    sumPosGs  += posGs.G;
                    sumPosGs2 += posGs.G * posGs.G;
                    vPosGs.push_back(posGs);
                }
                if (doG0s == true) doG0s = false;
                else               iG++;
            }
            double dEGs = (vPosGs.size() > 0 ? sumPosGs2 / sumPosGs : 0.0);
            std::sort(std::begin(vPosGs), std::end(vPosGs), [](const GTck &a, const GTck &b) { return a.G > b.G; });
                       
            // Register cs reqs in cReq and add canceled cnt
            GReq reqGcs;
            INT cntAvail = i.cnt;
            for (const auto &o : i.ord)
            {
                if (o.type == kOrdSell)
                {
                    int tck = i.P2Tck(o.p, kOrdSell); // 0-based tick
                    bool push  = false;
                    if ((tck >= 0) && (tck < (int)szTck)) // order price found in table
                    {
                        double Gc = i.G.at((std::size_t)tck).cs + dEGs; 
                        if (Gc > 0.0)
                        {
                            reqGcs.G = (FLOAT)(Gc * i.Ps0() * o.q * (1.0 - i.dSF()));
                            push = true;
                        }
                    }
                    if ((tck == (int)szTck) && (o.p > i.tbr.at(0).p)) // order price higher than any value in tb
                    {
                        reqGcs.G = 100.0f;
                        push = true;
                    }
                    if ((push == true) && (o.p > 0) && (o.q > 0)) 
                    {
                        cntAvail    += o.q;
                        reqGcs.type  = kReq_cs;
                        reqGcs.iM    = iM;
                        reqGcs.price = o.p;
                        reqGcs.quant = o.q;
                        cReq.push_back(reqGcs);
                    }
                }
            }
            
            // Register s reqs in oReq and add bal from s0
            GReq reqGs;
            INT cntLeft = cntAvail;
            for (const auto &req : vPosGs)
            {
                reqGs.price = i.Tck2P(req.tick, kOrdSell);
                if (i.Type() == kSecELW) reqGs.quant = 10 * (INT)std::round((double)cntAvail * req.G / sumPosGs / 10.0);
                else                     reqGs.quant =      (INT)std::round((double)cntAvail * req.G / sumPosGs       );
                reqGs.quant = (reqGs.quant > cntLeft ? cntLeft : reqGs.quant);
                if ((reqGs.price > 0) && (reqGs.quant > 0))
                {
                    cntLeft -= reqGs.quant;
                    if (req.tick == -1)
                    {
                        INT64 delta = (INT64)reqGs.price * reqGs.quant;
                        bal += delta - i.SFee(delta);
                        reqGs.G = (FLOAT)(i.G0.s                          * i.Ps0() * reqGs.quant * (1.0 - i.dSF()));
                    }
                    else
                        reqGs.G = (FLOAT)(i.G.at((std::size_t)req.tick).s * i.Ps0() * reqGs.quant * (1.0 - i.dSF()));
                    reqGs.type = kReq_s;
                    reqGs.iM   = iM;
                    oReq.push_back(reqGs);
                    if (cntLeft <= 0) break;
                }
            }            
        }

        // Buy reqs
        // Register positive Gb in intermediate buf and calculate EG
        std::vector<GTck> vPosGb;
        GTck posGb;
        double sumPosGb  = 0.0;
        double sumPosGb2 = 0.0;
        for (auto iM = std::begin(map); iM != std::end(map); iM++)
        {
            const auto &code_pItem = *iM;
            const auto &i = *code_pItem.second;
            bool doG0b = true;
            for (auto iG = std::begin(i.G); iG != std::end(i.G);) {
                if ((posGb.G   = (doG0b == true ? i.G0.b : iG->b)) > 0.0f)
                {
                    posGb.tick = (doG0b == true ? -1 : (int)(iG - std::begin(i.G)));
                    posGb.iM   = iM;
                    sumPosGb  += posGb.G;
                    sumPosGb2 += posGb.G * posGb.G;
                    vPosGb.push_back(posGb);
                }
                if (doG0b == true) doG0b = false;
                else               iG++;
            }
        }
        double lastRho = rho; // this will be used for investment in unused fund
        sumPosGb  += rhoWeight * rho;
        sumPosGb2 += rhoWeight * rho * rho;
        double dEGb = (vPosGb.size() > 0 ? sumPosGb2 / sumPosGb : 0.0);
        double dA = 1.0 / timeConst;
        rho = (1.0 - dA) * rho + dA * dEGb;
        std::sort(std::begin(vPosGb), std::end(vPosGb), [](const GTck &a, const GTck &b) { return a.G > b.G; });
        
        // Register cb reqs in cReq and add canceled bal
        GReq reqGcb;
        INT64 balAvail = bal;
        for (auto iM = std::begin(map); iM != std::end(map); iM++)
        {
            const auto &code_pItem = *iM;
            const auto &i = *code_pItem.second;
            for (const auto &o : i.ord)
            {
                if (o.type == kOrdBuy)
                {
                    int tck = i.P2Tck(o.p, kOrdBuy);
                    bool push  = false;
                    if ((tck >= 0) && (tck < (int)szTck)) // order price found in table
                    {
                        double Gb = i.G.at((std::size_t)tck).cb + dEGb; 
                        if (Gb > 0.0)
                        {
                            reqGcb.G = (FLOAT)(Gb * o.p * o.q * (1.0 + i.dBF()));
                            push = true;
                        }
                    }
                    if ((tck == (int)szTck) && (o.p < i.tbr.at(szTb - 1).p)) // order price lower than any value in tb
                    {
                        reqGcb.G = 100.0f;
                        push = true;
                    }
                    if ((push == true) && (o.p > 0) && (o.q > 0)) 
                    {
                        INT64 delta = (INT64)o.p * o.q;
                        balAvail += delta + i.BFee(delta);
                        reqGcb.type = kReq_cb;
                        reqGcb.iM   = iM;
                        reqGcb.price = o.p;
                        reqGcb.quant = o.q;
                        cReq.push_back(reqGcb);
                    }
                }
            }
        }
        
        // Register b reqs in oReq
        GReq reqGb;
        INT64 balLeft = balAvail - (INT64)std::round(balAvail * rhoWeight * lastRho / sumPosGb); // invest in unused fund first
        for (const auto &req : vPosGb)
        {
            auto &i = *(req.iM->second); // reference to Item
            reqGb.price = i.Tck2P(req.tick, kOrdBuy);
            INT64 balDist = (INT64)std::round((double)balAvail * req.G / sumPosGb);
            if (reqGb.price > 0) {
                if (i.Type() == kSecELW) reqGb.quant = 10 * (INT)std::round((double)balDist / (reqGb.price * (1.0 + i.dBF()) * 10));
                else                     reqGb.quant =      (INT)std::round((double)balDist / (reqGb.price * (1.0 + i.dBF())     ));
            }   else                     reqGb.quant = 0;
            INT64 delta = (INT64)reqGb.price * reqGb.quant;
            delta += i.BFee(delta);
            if (balLeft - delta < 0)
            {
                if (i.Type() == kSecELW) reqGb.quant -= 10;
                else                     reqGb.quant -=  1;
                delta  = (INT64)reqGb.price * reqGb.quant;
                delta += i.BFee(delta);
            }
            if ((reqGb.price > 0) && (reqGb.quant > 0) && (balLeft - delta >= 0))
            {
                balLeft -= delta;
                if (req.tick == -1) reqGb.G = FLOAT(i.G0.b                          * reqGb.price * reqGb.quant * (1.0 + i.dBF()));
                else                reqGb.G = FLOAT(i.G.at((std::size_t)req.tick).b * reqGb.price * reqGb.quant * (1.0 + i.dBF()));
                reqGb.type = kReq_b;
                reqGb.iM   = req.iM;
                oReq.push_back(reqGb);
                if (balLeft <= 0) break;
            }
        }


        // Sort and write out buffers
        std::sort(std::begin(cReq), std::end(cReq), [](const GReq &a, const GReq &b) { return a.G > b.G; });
        std::sort(std::begin(oReq), std::end(oReq), [](const GReq &a, const GReq &b) { return a.G > b.G; });
        
        bool bBreak = false;
        for (const auto &req : cReq)
        {
            if (true == (bBreak = CatReq(msg, (req.type == kReq_cb ? "cb" : "cs"), req.iM->first, req.price, req.quant))) 
                break;
        }
        if (bBreak == false)
        {
            for (const auto &req : oReq)
            {
                if      ( ((req.type == kReq_b ) || (req.type == kReq_s )) &&
                     (true == (bBreak = CatReq(msg, (req.type == kReq_b  ? "b"  : "s" ), req.iM->first, req.price, req.quant              ))) )
                    break;
                else if ( ((req.type == kReq_mb) || (req.type == kReq_ms)) &&
                     (true == (bBreak = CatReq(msg, (req.type == kReq_mb ? "mb" : "ms"), req.iM->first, req.price, req.quant, req.modprice))) )
                    break;
            }            
        }
    }
    
    if (msg.size() == 0) msg.append("\n");  // should have '\n' even for empty msg's
    
    if (logMsgOut.is_open() == true) // log raw output to server
        logMsgOut << "[t = " << time << "]\n" << msg; 
}

void Portfolio::WriteState()
{
    if (pathState.empty() == true) return;
    
    std::string filename(pathState);
    if (time <= timeInit)
        filename.append("client_ini.log");
    else if (time >= timeEnd - 60)
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
        
        fputs("\nCnt list\n", pf);
        int iCnt = 0;    // number of items in current line
        int nOrdTot = 0; // total number of orders
        for (const auto &code_pItem : map)
        {
            const auto &i = *code_pItem.second;
            if (i.cnt > 0)
            {
                fprintf(pf, "      {%s} %8d (%5d)", code_pItem.first.c_str(), i.Ps0(), i.cnt);
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
            for (const auto &code_pItem : map)
            {
                const auto &i = *code_pItem.second;
                for (const auto &o : i.ord)
                {
                    int tck = i.P2Tck(o.p, o.type); // 0-based tick
                    if (tck == szTck) tck = 98;     // display as 99 if not found
                    fprintf(pf, "[%s%2d] {%s} %8d (%5d)", (type == kOrdBuy ? "b" : "s"), tck + 1, code_pItem.first.c_str(), o.p, o.q);
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
        
        fprintf(pf, "\nOrd list (%2d)\n", nOrdTot);
        ListOrder(kOrdBuy) ; fputs("\n", pf);
        ListOrder(kOrdSell); fputs("\n", pf);
        
        fclose (pf);
    }
    else
        std::cerr << "Portfolio: state path not accessible" << std::endl;
}

void Portfolio::WritePosGCnt()
{
    if (pathState.empty() == true) return;
    
    std::string filename(pathState);
    filename.append("posGCnt.log");
    
    static std::array<int, szTb + 2> posGCnt = { 0 };
    for (const auto &code_pItem : map)
    {
        const auto &i = *code_pItem.second;
        if (i.G0.s > 0.0f) posGCnt[szTb + 0]++;
        if (i.G0.b > 0.0f) posGCnt[szTb + 1]++;
        std::ptrdiff_t tck = 0;
        for (const auto &gn : i.G)
        {
            if (gn.s > 0.0f) posGCnt[(std::size_t)(idxPs1 - tck  )]++;
            if (gn.b > 0.0f) posGCnt[(std::size_t)(idxPb1 + tck++)]++;
        }
    }
    
    FILE *pf = fopen(filename.c_str(), "w");
    if (pf != nullptr)
    {
        for (std::ptrdiff_t idx = 0; idx < szTb; idx++)
        {
            fprintf(pf, "     [%s%2d] %d\n", (idx <= idxPs1 ? "s" : "b"), (idx <= idxPs1 ? (int)idxPb1 - (int)idx : (int)idx - (int)idxPs1), posGCnt[(std::size_t)idx]);
            if (idx == idxPs1)
            {
                fprintf(pf, "     [%s%2d] %d\n", "s", 0, posGCnt[szTb + 0]);
                fprintf(pf, "     [%s%2d] %d\n", "b", 0, posGCnt[szTb + 1]);
            }        
        }
        fclose(pf);
    }
    else
        std::cerr << "Portfolio: state path not accessible" << std::endl;
}

}

#endif  /* SIBYL_PORTFOLIO_H_ */