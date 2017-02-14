/*
   Copyright 2017 Hosang Yoon

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "Simulation_dep.h"

#include <sys/stat.h>
#include <dirent.h>
#include <fstream>

#include "../../ReqType.h"

namespace sibyl
{

int Simulation_dep::LoadData(CSTR &cfgfile, CSTR &datapath)
{
    bool usingKOSPI = true;
    bool usingELW   = true;
    bool usingETF   = true;
    int  nELW = 0;   // specified in config file
    int  cntELW = 0; // actually added
    
    STR listInitCnt;
    STR listKOSPI;
    STR listETF;
    STR listNotKOSPI;
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
        else if ( name == "ETF_CODE") {
            if (val.empty() == false) listETF  = val;
            else                      usingETF = false;
        }
        else if ((name == "NOTKOSPI") && (val.empty() == false))
            listNotKOSPI = val;
        else if ((name == "DELAY_1H") && (val.empty() == false))
            listDelay1h = val; 
    }
    
    // length of code
    const int codeSize = 6;
    
    // auto add data
    STR path = datapath;
    if (path.empty() == false && path.back() != '/') path.append("/");
    STR pathETF = path + "ETF/";
    
    // last argument should be a 'new' pointer 
    auto OpenAndInsert = [&](CSTR &path, CSTR &code, ItemSim *p) {
        bool success = p->open(path, code);
        if (success == true)
        {
            auto it_bool = orderbook.items.insert(std::make_pair(code, std::unique_ptr<ItemSim>(p)));
            success = it_bool.second;
            if (success == false) // new pointer is deleted automatically in destructor
            {
                STR str = "Nonunique code {" + code + "} found";
                DisplayLoadError(str);
            }
        } else
            delete p;
        return success;
    };
    
    auto AutoAdd = [&](CSTR &path, SecType type) {
        struct stat sDir;
        if (-1 == stat(path.c_str(), &sDir)) return DisplayLoadError("Data path invalid");
        if (S_ISDIR(sDir.st_mode) == false)  return DisplayLoadError("Data path not a directory");
        
        DIR *pDir = opendir(path.c_str());
        if (pDir == nullptr)                 return DisplayLoadError("Data path inaccessible");
        
        struct dirent *pEnt;
        while ((pEnt = readdir(pDir)) != nullptr)
        {
            STR name = pEnt->d_name;
            STR ext  = ".txt";
            
            int nameSize = (int)name.size() - (int)ext.size(); 
            if ( (nameSize == codeSize || nameSize == codeSize + 1)             &&
                 (0 == name.compare(name.size() - ext.size(), ext.size(), ext)) )
            {
                STR code(name, 0, (std::size_t)codeSize);
                auto typeCur = ResolveSecType(path, code);
                if (typeCur == type) // this also filters invalid or already added items
                {
                    if (type == SecType::KOSPI) OpenAndInsert(path, code, new KOSPISim);
                    if (type == SecType::ETF  ) OpenAndInsert(path, code, new ETFSim  );    
                    if (type == SecType::ELW && (nELW == 0 || cntELW < nELW))
                    {
                        int type_expiry = ReadTypeExpiry(path, code);
                        if (type_expiry == 0) continue; // non-KOSPI200-related ELW
                        bool success = OpenAndInsert(path, code, new ELWSim((type_expiry > 0 ? OptType::call : OptType::put  ),
                                                                            (type_expiry > 0 ? type_expiry   : -type_expiry)));
                        if (success == true) cntELW++;
                    }
                }
            }
        }
        closedir(pDir);
        return 0;
    };
    
    // auto-add KOSPI
    if (usingKOSPI == true && listKOSPI.empty() == true) AutoAdd(path, SecType::KOSPI);
    
    // auto-add ELW
    if (usingELW   == true) AutoAdd(path, SecType::ELW);
    
    // auto-add ETF (in datapath/ETF directory)
    if (usingETF   == true && listETF.empty() == true) AutoAdd(pathETF, SecType::ETF);
     
    // manual add data (KOSPI)
    if (usingKOSPI == true && listKOSPI.size() > 0)
    {
        std::istringstream sKOSPI(listKOSPI);
        for (STR code; std::getline(sKOSPI, code, ';');)
            if (code.size() == codeSize)
                OpenAndInsert(path, code, new KOSPISim);
    }

    // manual add data (ETF)
    if (usingETF == true && listETF.size() > 0)
    {
        std::istringstream sETF(listETF);
        for (STR code; std::getline(sETF, code, ';');)
            if (code.size() == codeSize)
                OpenAndInsert(pathETF, code, new ETFSim);
    }

    // remove NOTKOSPI
    if (listNotKOSPI.size() > 0)
    {
        std::istringstream sBan(listNotKOSPI);
        for (STR code; std::getline(sBan, code, ';');)
        {
            if (code.size() == codeSize)
            {
                auto it = orderbook.items.find(code);
                if (it != std::end(orderbook.items) && it->second->Type() == SecType::KOSPI)
                    orderbook.items.erase(it); // destructors called on unique_ptr
            }
        }
    }

    // KOSPI200.txt
    dataKOSPI200.open(path + "KOSPI200.txt");

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
            if (date.size() == dateSize && 
                0 == path.compare(path.size() - dateSize, dateSize, date))
            {
                for (auto &code_pItem : orderbook.items)
                    code_pItem.second->SetDelay(3600);
                dataKOSPI200.SetDelay(3600);
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

int Simulation_dep::AdvanceTick()
{
    int timeTarget = orderbook.time + kTimeRates::secPerTick;
    
    do
    {
        ReadData(++orderbook.time);
        SimulateTrades();
    } while (orderbook.time < timeTarget);
    
    orderbook.UpdateRefInitBal();
    if (verbose == true) PrintState();
    
    nReqThisTick = 0;
    
    return (orderbook.time < kTimeBounds::end ? 0 : -1);
}

CSTR& Simulation_dep::BuildMsgOut()
{
    // calc pr, qr
    for (const auto &code_pItem : orderbook.items)
    {
        auto &i = *code_pItem.second;
        const auto &dataTr = i.TrData();
        if (dataTr.SumQ() > 0)   i.pr = (FLOAT)dataTr.SumPQ() / dataTr.SumQ();
        if (orderbook.time <= 0) i.pr = (FLOAT)i.Ps0(); // follow ps0 until 09:00:00
        i.qr = dataTr.SumQ();
        i.TrData().InitSum();
    }

    return orderbook.BuildMsgOut(true);
}

void Simulation_dep::PrintState()
{
    std::cout << "[t=" << orderbook.time << "] ----------------------------------------------------------------\n";
    std::cout << "bal " << orderbook.bal << "\n";
    auto evl = orderbook.Evaluate();
    std::cout << "evl " << evl.evalTot << "\n";
    
    const int  nItemPerLine = 5;
    const char itemSpacer[] = "    ";
    static char buf[1 << 8];
    
    std::cout << "cnt\n";
    
    int nItemCur = 0;
    for (const auto &code_pItem : orderbook.items)
    {
        const auto &i = *code_pItem.second;
        if (i.cnt > 0)
        {
            sprintf(buf, "                    {%s} %8d (%6d)", code_pItem.first.c_str(), i.Ps0(), i.cnt);
            std::cout << buf;
            if (nItemPerLine == ++nItemCur)
            {
                std::cout << "\n";
                nItemCur = 0;
            }
            else
                std::cout << itemSpacer;
        }
    }
    if (nItemCur != 0) std::cout << "\n";	
    
    std::cout << "ord\n";
    
    auto ListOrder = [&](const OrdType &type) {
        nItemCur = 0;
        for (const auto &code_pItem : orderbook.items)
        {
            const auto &i = *code_pItem.second;
            for (const auto &price_Order : i.ord)
            {
                const auto &o = price_Order.second;
                if (o.type == type && o.q > 0)
                {
                    int tck = i.P2Tck(o.p, o.type); // 0-based tick
                    if (tck == idx::tckN) tck = 98;     // display as 99 if not found
                    sprintf(buf, "[%6" PRId64 ",%6" PRId64 "|%s%2d] {%s} %8d (%6d)", o.B, o.M, (type == OrdType::buy ? "b" : "s"), tck + 1, code_pItem.first.c_str(), o.p, o.q);
                    std::cout << buf;
                    if (nItemPerLine == ++nItemCur)
                    {
                        std::cout << "\n";
                        nItemCur = 0;
                    }
                    else
                        std::cout << itemSpacer;
                }
            }
        }
        if (nItemCur != 0)  std::cout << "\n";
    };
    
    ListOrder(OrdType::buy) ;
    ListOrder(OrdType::sell);
    std::cout << std::endl;
}

void Simulation_dep::ReadData(int timeTarget)
{
    for (const auto &code_pItem : orderbook.items)
    {
        auto &i = *code_pItem.second;
        
        INT lastPb0 = i.Tck2P(-1, OrdType::buy );
        INT lastPs0 = i.Tck2P(-1, OrdType::sell);
         
        i.AdvanceTime(timeTarget);
        
        if (i.Tck2P(-1, OrdType::buy ) != lastPb0) i.depB0 = 0; // reset depletion if ps0/pb0 shifted
        if (i.Tck2P(-1, OrdType::sell) != lastPs0) i.depS0 = 0;
    }

    if (dataKOSPI200.is_open() == true) {
        dataKOSPI200.AdvanceTime(timeTarget);
        ELW<ItemSim>::kospi200 = std::fabs(dataKOSPI200[0]);
    }
}

void Simulation_dep::SimulateTrades()
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
                if (tck >= 0 && tck < idx::tckN) {
                    o.type = type;
                    vtro.push_back(o);
                }
            };
            AddIfInTbr(OrdType::buy );
            AddIfInTbr(OrdType::sell);
        }
        
        // Process order queue
        // Enforce OrdB <= tbqr for p1+ orders and OrdB = 0 for p0 orders
        for (auto &price_OrderSim : i.ord)
        {
            auto &o = price_OrderSim.second;
            INT tck = i.P2Tck(o.p, o.type);
            if (tck >= 0 && tck < idx::tckN) o.B = std::min(o.B, (INT64) i.Tck2Q(tck, o.type));
            else if (tck == -1)              o.B = 0;
        }
        
        // Make p0 orders with non-depleted tbqr
        auto MakeDeplete = [&](const OrdType &type) {
            auto &dep = (type == OrdType::buy ? i.depB0 : i.depS0);
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
                    if (o.type == type && o.q > 0) delta = (INT) std::max(o.M + o.q, (INT64) delta);
                }
                delta = std::min(qleft, delta);
                if (delta > 0)
                {
                    for (auto iOrd = first_last.first; iOrd != first_last.second; iOrd++)
                    {
                        const auto &o = iOrd->second;
                        if (o.type == type && o.q > 0)
                        {
                            if (verbose == true && delta > o.B + o.M) std::cout << "<q_0>" << std::endl;
                            PopBM(iItems, iOrd, delta);
                        }
                    }
                    dep += delta;
                }
            }
            else
                dep = tb0q;
        };
        MakeDeplete(OrdType::buy );
        MakeDeplete(OrdType::sell);
        
        // Make p0+ orders with trade events, filling lowest tick first
        for (const auto &t : vtro)
        {
            INT qleft = t.q;
            // int trTck = i.P2Tck(t.p, t.type); // (trTck >= 0 && trTck < idx::tckN) ensured above
            // for (int tck = -1; tck <= trTck; tck++)
            for (int tck = -1; tck <= 0; tck++) // limit to p1; appears to provide better accuracy
            {
                INT deltaq(0);
                const auto &first_last = i.ord.equal_range(i.Tck2P(tck, t.type));
                for (auto iOrd = first_last.first; iOrd != first_last.second; iOrd++)
                {
                    const auto &o = iOrd->second;
                    if (o.type == t.type && o.q > 0) deltaq = (INT) std::max(o.B + o.M + o.q, (INT64) deltaq);
                }
                deltaq = std::min(qleft, deltaq);
                if (deltaq > 0)
                {
                    for (auto iOrd = first_last.first; iOrd != first_last.second; iOrd++)
                    {
                        const auto &o = iOrd->second;
                        if (o.type == t.type && o.q > 0)
                        {
                            if (verbose == true && deltaq > o.B + o.M) std::cout << "<p_" << (tck + 1) << ">" << std::endl;
                            PopBM(iItems, iOrd, deltaq);
                        }
                    }
                    qleft -= deltaq;
                    if (qleft <= 0) break;
                }
            }
        }
        
        // Make <-1th orders
        for (auto iOrd = std::begin(i.ord); iOrd != std::end(i.ord); iOrd++)
        {
            const auto &o = iOrd->second;
            if (o.q > 0 && ((o.type == OrdType::buy  && o.p > i.Pb0()) || 
                            (o.type == OrdType::sell && o.p < i.Ps0()) ))
            {
                if (verbose == true) std::cout << "<p_-1>" << std::endl;
                orderbook.ApplyTrade(iItems, iOrd, PQ(o.p, o.q));
            }
        }
    }
}

int Simulation_dep::ExecuteNamedReq(NamedReq<OrderSim, ItemSim> req)
{
    if (req.q > 0 && nReqThisTick < kTimeRates::reqPerTick)
    {
        verify(req.type != ReqType::ca && req.type != ReqType::sa);
        
        if (req.type == ReqType::cb || req.type == ReqType::cs || req.type == ReqType::mb || req.type == ReqType::ms)
        {
            // q-asserts already in OrderBook's functions
            TrimM                (req.iItems, req.iOrd, req.q);
            orderbook.ApplyCancel(req.iItems, req.iOrd, req.q);
        }
        
        if (req.type == ReqType::b  || req.type == ReqType::s  || req.type == ReqType::mb || req.type == ReqType::ms)
        {
            OrderSim o;
            o.type = ((req.type == ReqType::b || req.type == ReqType::mb) ? OrdType::buy : OrdType::sell);
            o.p    = req.p;
            
            auto &i = *req.iItems->second;
            
            bool is0     = (o.p == i.Tck2P(-1, o.type));
            bool is0Anti = (o.p == i.Tck2P(-1, o.type == OrdType::buy ? OrdType::sell : OrdType::buy));
            
            auto &dep     = (o.type == OrdType::buy ? i.depB0 : i.depS0);
            auto &depAnti = (o.type == OrdType::buy ? i.depS0 : i.depB0);
            
            // immediate order (no B & M involved)
            if (is0 == true)
            {
                o.q = std::min(req.q, i.Tck2Q(-1, o.type) - dep);
                if (o.q > 0)
                {
                    auto iOrdTemp = orderbook.ApplyInsert(req.iItems, o);
                    if (verbose == true) std::cout << "<q_0>" << std::endl;
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
    return (++nReqThisTick < kTimeRates::reqPerTick ? 0 : -1);
}

int Simulation_dep::DisplayLoadError(CSTR &str)
{
    std::cerr << "Simulation::LoadData: " << str << std::endl;
    return -1;
}


SecType Simulation_dep::ResolveSecType(CSTR &path, CSTR &code)
{
    SecType type(SecType::null);
    
    if (std::end(orderbook.items) == orderbook.items.find(code))
    {
        // common
        std::ifstream tr(path + code + STR(".txt" ));
        std::ifstream tb(path + code + STR("t.txt"));
        
        // type-specific
        std::ifstream th(path + code + STR("g.txt")); // ELW only
        std::ifstream i (path + code + STR("i.txt")); // ELW only
        std::ifstream n (path + code + STR("n.txt")); // ETF only
        
        if (tr.is_open() == true && tb.is_open() == true)
        {
            if(th.is_open() == false && i.is_open() == false && n.is_open() == false) type = SecType::KOSPI;
            if(th.is_open() == true  && i.is_open() == true  && n.is_open() == false) type = SecType::ELW;
            if(th.is_open() == false && i.is_open() == false && n.is_open() == true ) type = SecType::ETF;
        }
        
        // files closed automatically in ~ifstream
    }  
    return type;
}

int Simulation_dep::ReadTypeExpiry(CSTR &path, CSTR &code)
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

INT64 Simulation_dep::GetM(it_itm_t<ItemSim> iItems, OrdType type, INT p) const
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

void Simulation_dep::PopBM(it_itm_t<ItemSim> iItems, it_ord_t<OrderSim> iOrd, INT q)
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

void Simulation_dep::TrimM(it_itm_t<ItemSim> iItems, it_ord_t<OrderSim> iOrd, INT q)
{
    const auto &first_last = iItems->second->ord.equal_range(iOrd->first);
    const auto type = iOrd->second.type;
    verify(iOrd != first_last.second); // iOrd must be in ItemSim pointed by iItems
    for (iOrd++; iOrd != first_last.second; iOrd++)
    {
        auto &o = iOrd->second;
        if (o.type == type && o.q > 0) // skip already emptied orders
        {
            o.M -= q;
            verify(o.M >= 0);
        }
    }
}

}