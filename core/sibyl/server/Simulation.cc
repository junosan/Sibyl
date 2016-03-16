
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>

#include "Simulation.h"

namespace sibyl
{

int Simulation::LoadData(CSTR &cfgfile, CSTR &datapath)
{
    bool usingKOSPI = true;
    bool usingELW   = true;
    int  nELW = 0;
    
    STR listInitCnt;
    STR listKOSPI;
    STR listBan;
    STR listDelay1h;
    
    // read config file
    std::ifstream sCfg(cfgfile);
    if (sCfg.is_open() == false) DisplayLoadError("Config file not accessible");
    for (STR line; std::getline(sCfg, line);)
    {
        if (line.back() == '\r') line.pop_back();
        auto posEq = line.find_first_of('=');
        if (posEq == std::string::npos) continue;
        auto namelen = std::min(posEq, line.find_first_of(' '));
        STR name(line, 0, namelen);
        STR val (line, posEq + 1 );
        if      ((name == "INIT_BAL") && (val.empty() == false))
            orderbook.bal = (INT64)std::stoll(val);
        else if ((name == "INIT_CNT") && (val.empty() == false))
            listInitCnt = val;
        else if ( name == "KOSPI_CL") {
            if (val.empty() == false) listKOSPI  = val;
            else                      usingKOSPI = false;
        }
        else if ( name == "ELW_NCNT") {
            if ((val.empty() == true) || ((nELW = std::stoi(val)) <= 0)) usingELW = false; 
        }
        else if ((name == "BAN_CODE") && (val.empty() == false))
            listBan = val;
        else if ((name == "DELAY_1H") && (val.empty() == false))
            listDelay1h = val; 
    }
    
    // length of code
    const int codeSize = 6;
    
    // auto add data
    STR path = datapath;
    if ((path.empty() == false) && (path[path.size() - 1] != '/')) path.append("/");
    
    struct stat sDir;
    if (-1 == stat(path.c_str(), &sDir)) return DisplayLoadError("Data path invalid");
    if (S_ISDIR(sDir.st_mode) == false)  return DisplayLoadError("Data path not a directory");
    
    if ( ((usingKOSPI == true) && (listKOSPI.empty() == true)) ||
          (usingELW   == true) ) // auto add 
    {
        int cntELW = 0;
        
        DIR *pDir = opendir(path.c_str());
        if (pDir == nullptr) return DisplayLoadError("Data path inaccessible");

        struct dirent *pEnt;
        while ((pEnt = readdir(pDir)) != nullptr)
        {
            STR name = pEnt->d_name;
            STR ext  = ".txt";
            
            int nameSize = (int)name.size() - (int)ext.size(); 
            if ( ( (nameSize == codeSize) || (nameSize == codeSize + 1) )      &&
                 (0 == name.compare(name.size() - ext.size(), ext.size(), ext)) )
            {
                STR code(name, 0, (std::size_t)codeSize);
                auto type = ResolveSecType(path, code);
                if (type == kSecNull) continue; // invalid or already added
                if ((type == kSecKOSPI) && (usingKOSPI == true))
                {
                    KOSPISim *p = new KOSPISim;
                    bool success = p->open(path, code);
                    if (success == true)
                    {
                        auto it_bool = orderbook.items.insert(std::make_pair(code, std::unique_ptr<ItemSim>(p)));
                        success = it_bool.second;
                    }
                    if (success == false) delete p;
                }
                if ((type == kSecELW  ) && (usingELW   == true) && ((nELW == 0) || (cntELW < nELW)))
                {
                    int type_expiry = ReadTypeExpiry(path, code);
                    if (type_expiry == 0) continue; // non-KOSPI200-related ELW
                    
                    ELWSim *p = new ELWSim((type_expiry > 0 ? kOptCall    : kOptPut     ),
                                           (type_expiry > 0 ? type_expiry : -type_expiry));
                    bool success = p->open(path, code);
                    if (success == true)
                    {
                        auto it_bool = orderbook.items.insert(std::make_pair(code, std::unique_ptr<ItemSim>(p)));
                        success = it_bool.second;
                    }
                    if (success == true) cntELW++;
                    else                 delete p;
                }
            }
        }
        closedir(pDir);
    }
    
    // manual add data (KOSPI only)
    if ((usingKOSPI == true) && (listKOSPI.size() > 0))
    {
        std::istringstream sKOSPI(listKOSPI);
        for (STR code; std::getline(sKOSPI, code, ';');)
        {
            if (code.size() == codeSize)
            {   
                KOSPISim *p = new KOSPISim;
                bool success = p->open(path, code);
                if (success == true)
                {
                    auto it_bool = orderbook.items.insert(std::make_pair(code, std::unique_ptr<ItemSim>(p)));
                    success = it_bool.second;
                }
                if (success == false) delete p;
            }
        }
    }

    // remove BAN_CODE
    if (listBan.size() > 0)
    {
        std::istringstream sBan(listBan);
        for (STR code; std::getline(sBan, code, ';');)
        {
            if (code.size() == codeSize)
            {
                auto it = orderbook.items.find(code);
                if (it != std::end(orderbook.items))
                    orderbook.items.erase(it); // destructors called on unique_ptr
            }
        }
    }

    // if data date is in DELAY_1H, set delay of 1 hour
    if (listDelay1h.size() > 0)
    {
        const int dateSize = 8;
        if (path.back() == '/') path.pop_back(); // path has already been validated
        if ( (path.size() < dateSize + 1) || 
             (path[path.size() - (dateSize + 1)] != '/') ||
             (false == std::all_of(std::begin(path) + (std::ptrdiff_t)path.size() - dateSize,
                                   std::end(path),
                                   (int (*)(int))std::isdigit)) )
            return DisplayLoadError("Invalid path format (*/YYYYMMDD)");
        
        std::istringstream sDelay1h(listDelay1h);
        for (STR date; std::getline(sDelay1h, date, ';');)
        {
            if ((date.size() == dateSize) && (0 == path.compare(path.size() - dateSize, dateSize, date)))
            {
                for (auto &code_pItem : orderbook.items)
                    code_pItem.second->SetDelay(3600);
                break;
            }
        }
    }
    
    // initialize initial cnt values
    if (listInitCnt.size() > 0)
    {
        std::istringstream sInitCnt(listInitCnt);
        for (STR item; std::getline(sInitCnt, item, ';');)
        {
            auto posSpace = item.find_first_of(' ');
            if ((item.size() > codeSize + 1) && (posSpace == codeSize))
            {
                STR code(item, 0, (std::size_t)codeSize);
                STR val (item, posSpace + 1);
                
                auto it = orderbook.items.find(code);
                if (it != std::end(orderbook.items)){
                    INT quant = std::stoi(val);
                    if (quant <= 0) {
                        DisplayLoadError("Invalid INIT_CNT cnt");
                        continue;
                    }
                    it->second->cnt = quant;
                } else  DisplayLoadError("Invalid INIT_CNT code");
            }
        } 
    }
    
    if (orderbook.items.size() == 0) return DisplayLoadError("0 items to simulate");
    if (verbose == true) std::cout << "[Done] Load data for " << orderbook.items.size() << " items" << std::endl; 
    
    return 0;
}

int Simulation::AdvanceTick()
{
    int timeTarget = orderbook.time + kTimeTickSec;
    do
    {
        ReadData(++orderbook.time);
        orderbook.UpdateRefInitBal();
        SimulateTrades();
    } while (orderbook.time < timeTarget);
    
    nReqThisTick = 0;
    
    if (verbose == true) std::cout << "[t=" << orderbook.time << "] ----------------------------------------------------------------" << std::endl;
    
    return (orderbook.time < timeBounds.end ? 0 : -1);
}

CSTR& Simulation::BuildMsgOut()
{
    // calc pr, qr
    for (const auto &code_pItem : orderbook.items)
    {
        auto &i = *code_pItem.second;
        const auto &dataTr = i.TrData();
        if (dataTr.SumQ() > 0) i.pr = (FLOAT)dataTr.SumPQ() / dataTr.SumQ();
        if (i.pr == 0.0f)      i.pr = (FLOAT)i.Ps0(); // the very first value
        i.qr = dataTr.SumQ();
    }
    
    const auto &msg = orderbook.BuildMsgOut(true);
    
    // InitSum for all data
    for (const auto &code_pItem : orderbook.items)
    {
        auto &i = *code_pItem.second;
        i.TrData().InitSum();
    }
    
    return msg;
}

void Simulation::ReadData(int timeTarget)
{
    for (const auto &code_pItem : orderbook.items)
    {
        auto &i = *code_pItem.second;
        
        INT lastPb0 = i.Tck2P(-1, kOrdBuy );
        INT lastPs0 = i.Tck2P(-1, kOrdSell);
         
        i.AdvanceTime(timeTarget);
        
        if (i.Tck2P(-1, kOrdBuy ) != lastPb0) i.depB0 = 0; // reset depletion if ps0/pb0 shifted
        if (i.Tck2P(-1, kOrdSell) != lastPs0) i.depS0 = 0;
    }
}

void Simulation::SimulateTrades()
{
    for (auto iItems = std::begin(orderbook.items); iItems != std::end(orderbook.items); iItems++)
    {
        auto &i = *iItems->second;
        
        // attach OrdType to PQ from TxtDataTr.VecTr()
        const auto &vtr = i.TrData().VecTr();
        std::vector<Order> vtro;
        for (const auto &t : vtr)
        {
            Order o(t.p, t.q);
            auto AddIfInTbr = [&](const OrdType &type) {
                INT tck = i.P2Tck(t.p, type);
                if ((tck >= 0) && (tck < szTck)) {
                    o.type = type;
                    vtro.push_back(o);
                }
            };
            AddIfInTbr(kOrdBuy );
            AddIfInTbr(kOrdSell);
        }
        
        // Process order queue
        // Enforce OrdB <= tbqr for p1+ orders to partially take cancels by others into account
        for (auto &price_OrderSim : i.ord)
        {
            auto &o = price_OrderSim.second;
            INT tck = i.P2Tck(o.p, o.type);
            if ((tck >= 0) && (tck < szTck)) o.B = std::min(o.B, (INT64)i.Tck2Q(tck, o.type));
        }
        
        // Make orders with trade events
        for (const auto &t : vtro)
        {
            const auto &first_last = i.ord.equal_range(t.p);
            for (auto iOrd = first_last.first; iOrd != first_last.second; iOrd++)
                if (iOrd->second.type == t.type) PopBM(iItems, iOrd, t.q);
        }
        
        // Make p0 orders with any remaining OrdB to partially take cancels by others into account
        for (auto iOrd = std::begin(i.ord); iOrd != std::end(i.ord); iOrd++)
        {
            auto &o = iOrd->second;
            if ((i.P2Tck(o.p, o.type) == -1) && (o.B > 0))
            {
                INT delta = (INT)o.B;
                o.B = 0;
                PopBM(iItems, iOrd, delta);
            }
        }
        
        // Make p0 orders with non-depleted tbqr
        auto MakeDeplete = [&](const OrdType &type) {
            auto &dep = (type == kOrdBuy ? i.depB0 : i.depS0);
            INT tb0q  = i.Tck2Q(-1, type);
            INT qleft = tb0q - dep;
            if (qleft > 0)
            {
                INT p = i.Tck2P(-1, type);
                INT delta(0);
                const auto &first_last = i.ord.equal_range(p);
                for (auto iOrd = first_last.first; iOrd != first_last.second; iOrd++)
                {
                    const auto &o = iOrd->second;
                    if (o.type == type) delta = (INT)std::max(o.M + o.q, (INT64)delta);
                }
                delta = std::min(qleft, delta);
                if (delta > 0)
                {
                    for (auto iOrd = first_last.first; iOrd != first_last.second; iOrd++)
                    {
                        const auto &o = iOrd->second;
                        if (o.type == type) PopBM(iItems, iOrd, delta);
                    }
                    dep += delta;
                }
            }
            else
                dep = tb0q;
        };
        MakeDeplete(kOrdBuy );
        MakeDeplete(kOrdSell);
        
        // Make <-1th orders (force to p0)
        for (auto iOrd = std::begin(i.ord); iOrd != std::end(i.ord); iOrd++)
        {
            auto &o = iOrd->second;
            if ((o.type == kOrdBuy ) && (o.p > i.Pb0())) orderbook.ApplyTrade(iItems, iOrd, PQ(i.Pb0(), o.q));
            if ((o.type == kOrdSell) && (o.p < i.Ps0())) orderbook.ApplyTrade(iItems, iOrd, PQ(i.Ps0(), o.q));
        }
    }
}

int Simulation::ExecuteNamedReq(NamedReq<OrderSim, ItemSim> req)
{
    if (nReqThisTick < kReqNPerTick)
    {
        assert((req.type != kReq_ca) && (req.type != kReq_sa));
        
        if ((req.type == kReq_cb) || (req.type == kReq_cs) || (req.type == kReq_mb) || (req.type == kReq_ms))
        {
            // q-asserts already in OrderBook's functions
            TrimM                (req.iItems, req.iOrd, req.q);
            orderbook.ApplyCancel(req.iItems, req.iOrd, req.q);
        }
        
        if ((req.type == kReq_b ) || (req.type == kReq_s ) || (req.type == kReq_mb) || (req.type == kReq_ms))
        {
            OrderSim o;
            o.type = (((req.type == kReq_b ) || (req.type == kReq_mb)) ? kOrdBuy : kOrdSell);
            o.p    = req.p;
            
            auto &i = *req.iItems->second;
            
            bool is0     = (o.p == i.Tck2P(-1, o.type));
            bool is0Anti = (o.p == i.Tck2P(-1, o.type == kOrdBuy ? kOrdSell : kOrdBuy));
            
            auto &dep     = (o.type == kOrdBuy ? i.depB0 : i.depS0);
            auto &depAnti = (o.type == kOrdBuy ? i.depS0 : i.depB0);
            
            // immediate order (no B & M involved)
            if (is0 == true)
            {
                o.q = std::min(req.q, i.Tck2Q(-1, o.type) - dep);
                if (o.q > 0)
                {
                    auto iOrdTemp = orderbook.ApplyInsert(req.iItems, o);
                    orderbook.ApplyTrade(req.iItems, iOrdTemp, PQ(o.p, o.q));
                    dep   += o.q;
                    req.q -= o.q;
                }
            }
            
            // delayed order (set B & M)
            if (req.q > 0)
            {
                o.q = req.q;
                o.B = i.Tck2Q(i.P2Tck(o.p, o.type), o.type) - (is0 == true ? dep : 0) - (is0Anti == true ? depAnti : 0);
                o.M = GetM(req.iItems, o.type, o.p);
                orderbook.ApplyInsert(req.iItems, o);
            }   
        }
    }
    return (++nReqThisTick < kReqNPerTick ? 0 : -1);
}

void Simulation::InitializeMembers()
{
    orderbook.SetTimeBounds(timeBounds);
    orderbook.time = -3600 + 600; // starts at 08:10:10 
}

int Simulation::DisplayLoadError(CSTR &str)
{
    std::cerr << "Simulation.LoadData: " << str << std::endl;
    return -1;
}


SecType Simulation::ResolveSecType(CSTR &path, CSTR &code)
{
    SecType type(kSecNull);
    
    if (std::end(orderbook.items) == orderbook.items.find(code))
    {
        std::ifstream tr(path + code + STR(".txt" ));
        std::ifstream tb(path + code + STR("t.txt"));
        std::ifstream th(path + code + STR("g.txt"));
        std::ifstream i (path + code + STR("i.txt"));
        
        if ((tr.is_open() == true) && (tb.is_open() == true))
        {
            if((th.is_open() == false) && (i.is_open() == false)) type = kSecKOSPI;
            if((th.is_open() == true ) && (i.is_open() == true )) type = kSecELW;
        }
        
        // files closed automatically in ~ifstream
    }  
    return type;
}

int Simulation::ReadTypeExpiry(CSTR &path, CSTR &code)
{    
    int type = 0;
    int expiry = 0;
    
    std::ifstream sInfo(path + code + STR("i.txt"));
    if (sInfo.is_open() == true)
    {
        for (STR line; std::getline(sInfo, line);)
        {
            if (line.back() == '\r') line.pop_back();
            auto posEq = line.find_first_of('=');
            if (posEq == std::string::npos) continue;
            auto namelen = std::min(posEq, line.find_first_of(' '));
            STR name(line, 0, namelen);
            STR val (line, posEq + 1 );
            if      ((name == "TYPE"  ) && (val.empty() == false))
            {
                if      (val == "c") type = +1;
                else if (val == "p") type = -1;
            }
            else if ((name == "EXPIRY") && (val.empty() == false))
                expiry = std::stoi(val);
            else if ((name == "NAME"  ) && (val.empty() == false))
            {
                if (std::string::npos == val.find("KOSPI200")) 
                    return 0; // skip this item
            }
        }
    }

    return type * expiry;
}

INT64 Simulation::GetM(it_itm_t<ItemSim> iItems, OrdType type, INT p) const
{
    INT64 M(0);
    const auto &first_last = iItems->second->ord.equal_range(p);
    for (auto iP = first_last.first; iP != first_last.second; iP++)
    {
        const auto &o = iP->second;
        if (o.type == type) M += o.q; // o.q == 0 doesn't do anything
    }
    return M; 
}

void Simulation::PopBM(it_itm_t<ItemSim> iItems, it_ord_t<OrderSim> iOrd, INT q)
{
    auto &o = iOrd->second;
    if (o.q > 0) // skip already emptied orders
    {
        INT delta = (INT)std::min((INT64)q, o.B); 
        q   -= delta;
        o.B -= delta;
        if (q > 0)
        {
            delta = (INT)std::min((INT64)q, o.M);
            q   -= delta;
            o.M -= delta;
            if (q > 0)
            {
                q = std::min(q, o.q);
                orderbook.ApplyTrade(iItems, iOrd, PQ(o.p, q));
            }
        }
    }
}

void Simulation::TrimM(it_itm_t<ItemSim> iItems, it_ord_t<OrderSim> iOrd, INT q)
{
    const auto &first_last = iItems->second->ord.equal_range(iOrd->first);
    const auto type = iOrd->second.type;
    assert(iOrd != first_last.second); // iOrd must be in ItemSim pointed by iItems
    for (iOrd++; iOrd != first_last.second; iOrd++)
    {
        auto &o = iOrd->second;
        if ((o.type == type) && (o.q > 0)) // skip already emptied orders
        {
            o.M -= q;
            assert(o.M >= 0);
        }
    }
}

}