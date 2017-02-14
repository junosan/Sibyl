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

#include "TxtData.h"

#include <iostream>

namespace sibyl
{

bool TxtData::open(CSTR &filename_)
{
    pf = fopen(filename_.c_str(), "r");
    if (pf != nullptr)
    {
        filename = filename_;
        AdvanceLine();
        if (open_bool == true) Cur2Last(false);
    }
    else
        std::cerr << "TxtData.open: " << filename_ << " inaccessible" << std::endl;
    return open_bool;
}

bool TxtData::is_open() const
{
    return open_bool;
}

void TxtData::AdvanceTime(int timeTarget)
{
    while (open_bool == true && time < timeTarget) {
        Cur2Last(true);
        AdvanceLine();
    }
}

void TxtData::AdvanceLine()
{
    constexpr static std::size_t szBuf = (1 << 12);
    static char bufLine[szBuf];
    
    char *pcLine = fgets(bufLine, szBuf, pf);
    bool success = true;
    bool invalid = false;
    if (pcLine == NULL) success = false; // end of file
    if (success == true)
    {
        if (1 != sscanf(pcLine, "%d", &time))
        {
            success = false;
            invalid = true;
        }
        else
            time = Txt2Time(time) - delay;
    }
    if (success == true)
    {
        if (0 != ReadLine(pcLine)) {
            success = false;
            invalid = true;
        }
    }
    if (success == true) open_bool = true;
    else
    {
        if (invalid == true) std::cerr << "TxtData.AdvanceLine: invalid format in " << filename << '\n' << pcLine << std::endl;
        open_bool = false;
    }
}

void TxtData::SetDelay(int d)
{
    verify(d >= 0);
    delay = d;
}

int TxtData::Txt2Time(int txt)
{
    int h = txt / 10000;
    int m = (txt - h * 10000) / 100; 
    int s = txt - h * 10000 - m * 100;
    return (h - 9) * 3600 + m * 60 + s;
}


    /* =========================================== */
    /*                  TxtDataTr                  */
    /* =========================================== */

int TxtDataTr::ReadLine(const char *pcLine)
{
    int temp;
    bool invalid = false;
    if (5 == sscanf(pcLine, "%d %d %d %d %d", &temp, &cur.q, &cur.p, &cur.ps1, &cur.pb1))
    {
        if (time >= 0)
        {
            cur.q   = std::abs(cur.q);
            cur.p   = std::abs(cur.p);
            cur.ps1 = std::abs(cur.ps1);
            cur.pb1 = std::abs(cur.pb1);
        }
        else // ignore trade data before 09:00:00 as their ps1/pb1 values are invalid
            cur.q = cur.p = cur.ps1 = cur.pb1 = 0;
    }
    else
        invalid = true;
    return (invalid == false ? 0 : -1);
}

void TxtDataTr::Cur2Last(bool sum)
{
    last = cur;
    
    if (sum == true)
    {
        sumQ  += (INT64)last.q;
        sumPQ += (INT64)last.p * last.q;
        vecTr.push_back(PQ(last.p, last.q));
    }
    else // first line of file
    {
        last.ps1 = last.pb1 = 0; // prevent >= 09:00:00 values from interfering 
    }
}


    /* =========================================== */
    /*                  TxtDataTb                  */
    /* =========================================== */

int TxtDataTb::ReadLine(const char *pcLine)
{
    int temp;
    bool invalid = false;
    if ( ((type == SecType::KOSPI || type == SecType::ETF) &&
         (41 == sscanf(pcLine, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                                &temp,
                                &cur[ 0].p, &cur[ 0].q,
                                &cur[ 1].p, &cur[ 1].q,
                                &cur[ 2].p, &cur[ 2].q,
                                &cur[ 3].p, &cur[ 3].q,
                                &cur[ 4].p, &cur[ 4].q, 
                                &cur[ 5].p, &cur[ 5].q,
                                &cur[ 6].p, &cur[ 6].q,
                                &cur[ 7].p, &cur[ 7].q,
                                &cur[ 8].p, &cur[ 8].q,
                                &cur[ 9].p, &cur[ 9].q,
                                &cur[10].p, &cur[10].q,
                                &cur[11].p, &cur[11].q,
                                &cur[12].p, &cur[12].q,
                                &cur[13].p, &cur[13].q,
                                &cur[14].p, &cur[14].q, 
                                &cur[15].p, &cur[15].q,
                                &cur[16].p, &cur[16].q,
                                &cur[17].p, &cur[17].q,
                                &cur[18].p, &cur[18].q,
                                &cur[19].p, &cur[19].q ))) ||
         ((type == SecType::ELW) &&
          (61 == sscanf(pcLine, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                                &temp,
                                &cur[ 0].p, &cur[ 0].q, &temp,
                                &cur[ 1].p, &cur[ 1].q, &temp,
                                &cur[ 2].p, &cur[ 2].q, &temp,
                                &cur[ 3].p, &cur[ 3].q, &temp,
                                &cur[ 4].p, &cur[ 4].q, &temp,
                                &cur[ 5].p, &cur[ 5].q, &temp,
                                &cur[ 6].p, &cur[ 6].q, &temp,
                                &cur[ 7].p, &cur[ 7].q, &temp,
                                &cur[ 8].p, &cur[ 8].q, &temp,
                                &cur[ 9].p, &cur[ 9].q, &temp,
                                &cur[10].p, &cur[10].q, &temp,
                                &cur[11].p, &cur[11].q, &temp,
                                &cur[12].p, &cur[12].q, &temp,
                                &cur[13].p, &cur[13].q, &temp,
                                &cur[14].p, &cur[14].q, &temp,
                                &cur[15].p, &cur[15].q, &temp,
                                &cur[16].p, &cur[16].q, &temp,
                                &cur[17].p, &cur[17].q, &temp,
                                &cur[18].p, &cur[18].q, &temp,
                                &cur[19].p, &cur[19].q, &temp ))) )
    {
        for (auto &t : cur)
        {
            t.p = std::abs(t.p);
            t.q = std::abs(t.q);
        }
    }
    else
        invalid = true;
    return (invalid == false ? 0 : -1);
}

void TxtDataTb::Cur2Last(bool sum)
{
    last = cur;
}

}
