#ifndef SIBYL_CLIENT_REWARDMODEL_H_
#define SIBYL_CLIENT_REWARDMODEL_H_

#include <fstream>
#include <map>

#include "../Security.h"
#include "Portfolio.h"
#include "Model.h"
#include "Reward.h"

namespace sibyl
{

class RewardModel : public Model
{
public:  
    void SetParams(double timeConst_, double rhoWeight_, double rhoInit_);

    // for ref
    void SetRefPath(CSTR &path);
    void GetRefData();
    
    // for rnn 
    std::vector<Reward>& GetRewardVec(); // returns vec with correct size (no content)
    void                 SetRewardVec(const std::vector<Reward> &vec); // put in here after filling content

    // virtuals from Model; to be called by Trader
    void  SetStateLogPaths(CSTR &state, CSTR &log);
    CSTR& BuildMsgOut     ();
    
    RewardModel() : timeConst(0.0), // for initialization check
                    rhoWeight(0.0), rho(0.0), isFirstTick(true) {}
private:
    double timeConst;
    double rhoWeight;
    double rho;

    std::map<STR, Reward> rewards;
    void InitCodes(); // create rewards entries with codes from first msg
    
    // for ref
    STR pathData;
    std::map<STR, std::unique_ptr<FILE, int(*)(FILE*)>> mfRef; // for binary log of G values

    // for rnn
    std::vector<Reward> vecRewards;

    // for logging
    STR pathState;
    STR pathLog;
    
    bool isFirstTick;
    void InitGLogs   ();
    void WriteGLogs  (); // writes only if  log  path was set
    void WritePosGCnt(); // writes only if state path was set
    
    std::ofstream logMsgOut; // req msg
    std::ofstream logVecIn;  // G
    std::map<STR, std::unique_ptr<FILE, int(*)(FILE*)>> mfLogRef; // for binary log of G values
    
    int nMsgCurTick;
    char bufLine[1 << 10];
    bool CatReq(const char *pcMsgType, CSTR &code, INT price, INT quant, INT modprice = 0)
    {
        if (modprice > 0) sprintf(bufLine, "%s %s %d %d %d\n", pcMsgType, code.c_str(), price, quant, modprice);
        else              sprintf(bufLine, "%s %s %d %d\n"   , pcMsgType, code.c_str(), price, quant);
        msg.append(bufLine);
        if (kReqNPerTick == ++nMsgCurTick)
            return true;
        return false;        
    }
    STR msg;
};

void RewardModel::SetParams(double timeConst_, double rhoWeight_, double rhoInit_)
{
    assert((timeConst_ > 0.0) && (rhoWeight_ >= 0.0) && (rhoInit_ >= 0.0));
    timeConst = timeConst_; rhoWeight = rhoWeight_; rho = rhoInit_;
}

void RewardModel::SetStateLogPaths(CSTR &state, CSTR &log)
{
    pathState = state;
    if ((pathState.empty() == false) && ('/' != pathState[pathState.size() - 1])) pathState.append("/");
    pathLog   = log;
    if ((pathLog.empty()   == false) && ('/' != pathLog  [pathLog.size()   - 1])) pathLog.append("/");
    
    if (pathLog.empty() == false)
    {
        logMsgOut.open(pathLog + "msg_out.log", std::ofstream::trunc);
        logVecIn.open (pathLog + "vec_in.log" , std::ofstream::trunc);
        if ((logMsgOut.is_open() == false) || (logVecIn.is_open() == false))
            std::cerr << "RewardModel: log path not accessible" << std::endl;
    }
}

void RewardModel::SetRefPath(CSTR &path)
{
    pathData = path;
    if ('/' != pathData[pathData.size() - 1]) pathData.append("/");
}

void RewardModel::InitCodes()
{
    assert(pPortfolio != nullptr);
    for (const auto &code_pItem : pPortfolio->items)
    {
        auto it_bool = rewards.insert(std::make_pair(code_pItem.first, Reward()));
        assert(it_bool.second == true);
    }
    InitGLogs();
}

void RewardModel::InitGLogs()
{
    if (pathLog.empty() == false)
    {
        for (const auto &code_reward : rewards)
        {
            STR filename = pathLog + code_reward.first + ".ref";
            FILE *pf = fopen(filename.c_str(), "wb");
            assert(pf != nullptr); // kill, as fclose(nullptr) is a crash anyways
            auto it_bool = mfLogRef.insert(std::make_pair(code_reward.first, std::unique_ptr<FILE, int(*)(FILE*)>(pf, fclose)));
            assert(it_bool.second == true);
        }
    }
}

void RewardModel::GetRefData()
{
    if (isFirstTick == true)
    {
        InitCodes(); 
        for (const auto &code_reward : rewards)
        {
            STR filename = pathData + code_reward.first + STR(".ref");
            FILE *pf = fopen(filename.c_str(), "rb");
            if (pf == nullptr)
            {
                std::cerr << filename << " not found" << std::endl;
                return;
            }
            auto it_bool = mfRef.insert(std::make_pair(code_reward.first, std::unique_ptr<FILE, int(*)(FILE*)>(pf, fclose)));
            assert(it_bool.second == true);
        }
        isFirstTick = false;
    }
    if ((pPortfolio->time >= timeBounds.init) && (pPortfolio->time < timeBounds.stop))
    {
        const std::ptrdiff_t dimRef = 42;
        float bufRef[dimRef];

        for (auto &code_reward : rewards)
        {
            auto &r = code_reward.second;
            auto imf = mfRef.find(code_reward.first);
            assert(imf != std::end(mfRef));
            std::size_t szRead = fread(bufRef, sizeof(float), dimRef, &*(imf->second));
            if (szRead == dimRef)
            {
                r.G0.s = bufRef[0];
                r.G0.b = bufRef[1];
                std::ptrdiff_t tck = 0;
                for (auto &gn : r.G)
                {
                    gn.s  = bufRef[ 2 + tck  ];
                    gn.b  = bufRef[12 + tck  ]; 
                    gn.cs = bufRef[22 + tck  ];
                    gn.cb = bufRef[32 + tck++]; 
                }
            }
            else
                break;
            
            auto &i = *(pPortfolio->items.find(code_reward.first)->second);
            if (i.Type() == kSecKOSPI)
            {
                for (int iB = 0; iB < (int)dimRef; iB++)
                {
                    if ((std::abs(bufRef[iB]) > 1.0f) && (std::abs(bufRef[iB]) < 100.0f)) {
                        sprintf(bufLine, "Warning: [%5d] {%s} G = %f too large (iB %d)", pPortfolio->time, code_reward.first.c_str(), bufRef[iB], iB);
                        std::cerr << bufLine << std::endl;
                    }
                }
            }
        }
    }
}

std::vector<Reward>& RewardModel::GetRewardVec()
{
    if (isFirstTick == true)
    {
        InitCodes();
        isFirstTick = false;
    }
    if (vecRewards.size() != rewards.size()) vecRewards.resize(rewards.size());
    for (auto &reward : vecRewards) reward.SetZero();
    return vecRewards;
}

void RewardModel::SetRewardVec(const std::vector<Reward> &vec)
{
    assert(rewards.size() == vec.size());
    std::size_t codeIdx = 0;
    for (auto &code_reward : rewards)
    {
        code_reward.second = vec[codeIdx];
        codeIdx++; 
    }
}

// shorthand for Catalog.map's iterator
template <class TItem>
using it_map_t = typename std::map<STR, std::unique_ptr<TItem>>::iterator; 

class GTck {
public:
    FLOAT G;
    it_map_t<Item> iM;
    int tick;          // 0-based index of price (-1: Gs0/Gb0, and so on)
    GTck() : G(0.0f), tick(0) {}
};

class GReq {
public:
    FLOAT G;           // GTck * price * quant * (1 +- fee/tax)
    ReqType type;    
    it_map_t<Item> iM;
    INT price;         // actual price
    INT quant;         // actual quantity
    INT modprice;      // actual mod price
    GReq() : G(0.0f), type(kReqNull), price(0), quant(0), modprice(0) {}
};

CSTR& RewardModel::BuildMsgOut()
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
    
    const auto &time  = pPortfolio->time;
          auto  bal   = pPortfolio->bal;
          auto &items = pPortfolio->items;
    
    if (pathState.empty() == false) {
        WritePosGCnt();
    }
    if ((logVecIn.is_open() == true) && // log G values
        (time >= timeBounds.init) && (time < timeBounds.stop)) {
        WriteGLogs();
    }
    
    const int timeCancelAll  = 21000 + 600 + 60;
    
    // Stop buying anything between 14:30 and 14:50
    if ((time >= timeBounds.stop - 1200) && (time < timeBounds.stop))
    {
        for (auto &code_reward : rewards)
        {
            auto &r = code_reward.second;
            r.G0.b    = -100.0f;
            for (auto &gn : r.G)
            {
                gn.b  = -100.0f;
                gn.cb =  100.0f;
            }
        }
    }
    
    // Don't do anything between 14:50 and 15:01
    if ((time >= timeBounds.stop) && (time < timeCancelAll))
    {
        for (auto &code_reward : rewards)
        {
            auto &r = code_reward.second;
            r.G0.b    = -100.0f;
            r.G0.s    = -100.0f;
            for (auto &gn : r.G)
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
        for (auto &code_reward : rewards)
        {
            auto &r = code_reward.second;
            r.G0.b    = -100.0f;
            r.G0.s    = -100.0f;
            for (auto &gn : r.G)
            {
                gn.b  = -100.0f;
                gn.cb =  100.0f;
                gn.s  = -100.0f;
                gn.cs =  100.0f;
            }
        }        
    }
    
    // between 09:00:int and 15:01:00
    if (time >= timeBounds.init)
    {
        // Annihilate self-contradictory inputs
        auto Annihilate = [](FLOAT &a, FLOAT &b) {
            if ((a > 0.0f) && (b > 0.0f)) {
                if (a > b) { a = a - b; b = 0.0f; }
                else       { b = b - a; a = 0.0f; }
            } 
        };
        for (auto &code_reward : rewards)
        {
            auto &r = code_reward.second;
            Annihilate(r.G0.b, r.G0.s);
            for (auto &gn : r.G)
            {
                Annihilate(gn.b, gn.cb);
                Annihilate(gn.s, gn.cs);
            }
        }
        
        nMsgCurTick = 0;
        std::vector<GReq> cReq; // Buf for cs, cb
        std::vector<GReq> oReq; // Buf for s, ms, b, mb 

        // Sell reqs
        for (auto iM = std::begin(items); iM != std::end(items); iM++)
        {
            const auto &code_pItem = *iM;
            const auto &i = *code_pItem.second;
            const auto &r = rewards.find(code_pItem.first)->second;

            // Register positive Gs in intermediate buf and calculate EG 
            std::vector<GTck> vPosGs;
            GTck posGs;
            double sumPosGs  = 0.0;
            double sumPosGs2 = 0.0;
            bool doG0s = true;
            for (auto iG = std::begin(r.G); iG != std::end(r.G);) {
                if ((posGs.G   = (doG0s == true ? r.G0.s : iG->s)) > 0.0f)
                {
                    posGs.tick = (doG0s == true ? -1 : (int)(iG - std::begin(r.G)));
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
            for (const auto &price_Order : i.ord)
            {
                const auto &o = price_Order.second;
                if (o.type == kOrdSell)
                {
                    int tck = i.P2Tck(o.p, kOrdSell); // 0-based tick
                    bool push  = false;
                    if ((tck >= 0) && (tck < (int)szTck)) // order price found in table
                    {
                        double Gc = r.G.at((std::size_t)tck).cs + dEGs; 
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
                        reqGs.G = (FLOAT)(r.G0.s                          * i.Ps0() * reqGs.quant * (1.0 - i.dSF()));
                    }
                    else
                        reqGs.G = (FLOAT)(r.G.at((std::size_t)req.tick).s * i.Ps0() * reqGs.quant * (1.0 - i.dSF()));
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
        for (auto iM = std::begin(items); iM != std::end(items); iM++)
        {
            const auto &code_pItem = *iM;
            const auto &i = *code_pItem.second;
            const auto &r = rewards.find(code_pItem.first)->second;
            
            bool doG0b = true;
            for (auto iG = std::begin(r.G); iG != std::end(r.G);) {
                if ((posGb.G   = (doG0b == true ? r.G0.b : iG->b)) > 0.0f)
                {
                    posGb.tick = (doG0b == true ? -1 : (int)(iG - std::begin(r.G)));
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
        for (auto iM = std::begin(items); iM != std::end(items); iM++)
        {
            const auto &code_pItem = *iM;
            const auto &i = *code_pItem.second;
            const auto &r = rewards.find(code_pItem.first)->second;
            
            for (const auto &price_Order : i.ord)
            {
                const auto &o = price_Order.second;
                if (o.type == kOrdBuy)
                {
                    int tck = i.P2Tck(o.p, kOrdBuy);
                    bool push  = false;
                    if ((tck >= 0) && (tck < (int)szTck)) // order price found in table
                    {
                        double Gb = r.G.at((std::size_t)tck).cb + dEGb; 
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
            const auto &r = rewards.find(req.iM->first)->second;
            
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
                if (req.tick == -1) reqGb.G = FLOAT(r.G0.b                          * reqGb.price * reqGb.quant * (1.0 + i.dBF()));
                else                reqGb.G = FLOAT(r.G.at((std::size_t)req.tick).b * reqGb.price * reqGb.quant * (1.0 + i.dBF()));
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
            if (true == (bBreak = CatReq((req.type == kReq_cb ? "cb" : "cs"), req.iM->first, req.price, req.quant))) 
                break;
        }
        if (bBreak == false)
        {
            for (const auto &req : oReq)
            {
                if      ( ((req.type == kReq_b ) || (req.type == kReq_s )) &&
                     (true == (bBreak = CatReq((req.type == kReq_b  ? "b"  : "s" ), req.iM->first, req.price, req.quant              ))) )
                    break;
                else if ( ((req.type == kReq_mb) || (req.type == kReq_ms)) &&
                     (true == (bBreak = CatReq((req.type == kReq_mb ? "mb" : "ms"), req.iM->first, req.price, req.quant, req.modprice))) )
                    break;
            }            
        }
    }
    
    if (msg.size() == 0) msg.append("\n");  // should have '\n' even for empty msg's
    
    if (logMsgOut.is_open() == true) // log raw output to server
        logMsgOut << "[t=" << time << "]\n" << msg;
        
    return msg;
}

void RewardModel::WritePosGCnt()
{
    if (pathState.empty() == true) return;
    
    STR filename(pathState);
    filename.append("posGCnt.log");
    
    static std::array<int, szTb + 2> posGCnt = { 0 };
    for (const auto &code_reward : rewards)
    {
        const auto &r = code_reward.second;
        if (r.G0.s > 0.0f) posGCnt[szTb + 0]++;
        if (r.G0.b > 0.0f) posGCnt[szTb + 1]++;
        std::ptrdiff_t tck = 0;
        for (const auto &gn : r.G)
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

void RewardModel::WriteGLogs()
{
    if ((logVecIn.is_open() == true) && 
        (pPortfolio->time >= timeBounds.init) && (pPortfolio->time < timeBounds.stop))
    {
        logVecIn << "[t=" << pPortfolio->time << "]\n";
        for (const auto &code_reward : rewards)
        {
            // text log
            const auto &r = code_reward.second;
            sprintf(bufLine, "{%s}\n"                 , code_reward.first.c_str()); logVecIn << bufLine;
            sprintf(bufLine, "Gs0\t%+.3e\n"           , r.G0.s);                    logVecIn << bufLine;
            sprintf(bufLine, "Gb0\t%+.3e\n"           , r.G0.b);                    logVecIn << bufLine;
            sprintf(bufLine, "     \tGs/b\t\tGcs/cb\n");                            logVecIn << bufLine;
            for (std::ptrdiff_t idx = 0; idx <= idxPs1; idx++) {
                sprintf(bufLine, "[s%2d]\t%+.3e\t%+.3e\n", (int)(1 + idxPs1 - idx), r.G[(std::size_t)(idxPs1 - idx)].s, r.G[(std::size_t)(idxPs1 - idx)].cs);
                logVecIn << bufLine;
            }
            for (std::ptrdiff_t idx = 0; idx < szTck; idx++) {
                sprintf(bufLine, "[b%2d]\t%+.3e\t%+.3e\n", (int)(1 + idx)         , r.G[(std::size_t)idx].b           , r.G[(std::size_t)idx].cb           );
                logVecIn << bufLine;
            }
            
            // binary log
            std::size_t szDim = 42;
            float data[szDim];
            auto imf = mfLogRef.find(code_reward.first);
            assert(imf != std::end(mfLogRef));
            FILE *pf = &*(imf->second);
            data[0] = r.G0.s;
            data[1] = r.G0.b;
            
            std::ptrdiff_t tck = 0;
            for (const auto &gn : r.G)
            {
                data[ 2 + tck  ] = gn.s;
                data[12 + tck  ] = gn.b; 
                data[22 + tck  ] = gn.cs;
                data[32 + tck++] = gn.cb; 
            }
            fwrite(data, sizeof(float), szDim, pf);
        }
    }
}

}

#endif /* SIBYL_CLIENT_REWARDMODEL_H_ */