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

#ifndef SIBYL_SERVER_SIMULATION_TXTDATA_H_
#define SIBYL_SERVER_SIMULATION_TXTDATA_H_

#include <vector>
#include <sstream>
#include <cstring>

#include "../../time_common.h"
#include "../../Security.h"

namespace sibyl
{

class TxtData
{
public:
    bool open(CSTR &filename_);
    bool is_open() const;
    void AdvanceTime(int timeTarget); // TxtDataTr requires InitSum | InitVecTr prior to this
    void SetDelay(int d);
    
    TxtData() : time(kTimeBounds::null), delay(0), pf(nullptr), open_bool(false) {}
    ~TxtData() { if (pf != nullptr) fclose(pf); }
protected:
    virtual int  ReadLine(const char *pcLine) = 0; // returns non-0 to signal invalid format
    virtual void Cur2Last(bool sum)           = 0; // backup 'cur' to 'last' & and sum last (if applicable)
    int time, delay; // note: read only for derived classes
private:
    void AdvanceLine(); // read new line to 'cur', read time, check eof & formatting error
    static int Txt2Time(int txt);
    FILE *pf;
    STR filename;
    bool open_bool;
};


    /* =========================================== */
    /*                  TxtDataTr                  */
    /* =========================================== */

class TxtDataTr : public TxtData
{
public:
    void InitSum  () { sumQ = sumPQ = 0; } // every kTimeRates::secPerTick sec
    void InitVecTr() { vecTr.clear();    } // every           1            sec
    const std::vector<PQ>& VecTr() const { return vecTr; }
    const INT64&           SumQ () const { return sumQ; }
    const INT64&           SumPQ() const { return sumPQ; }
    const INT&             TrPs1() const { return last.ps1; }
    const INT&             TrPb1() const { return last.pb1; }
    
    TxtDataTr() : sumQ(0), sumPQ(0), cur{}, last{} {}
private:
    // virtuals from TxtData
    int  ReadLine(const char *pcLine);
    void Cur2Last(bool sum);
    
    // sums    
    INT64 sumQ, sumPQ;
    std::vector<PQ> vecTr;
    
    struct { INT p, q, ps1, pb1; } cur, last;
};


    /* =========================================== */
    /*                  TxtDataTb                  */
    /* =========================================== */

class TxtDataTb : public TxtData
{
public:
    const std::array<PQ, idx::szTb>& Tb() { return last; }
    TxtDataTb(SecType type_) : type(type_) {}
private:
    // virtuals from TxtData
    int  ReadLine(const char *pcLine);
    void Cur2Last(bool sum);

    SecType type;

    std::array<PQ, idx::szTb> cur, last;
};


    /* ============================================ */
    /*                  TxtDataVec                  */
    /* ============================================ */

template <class T>
class TxtDataVec : public TxtData
{
public:
    const T& operator[](std::size_t pos) { return last.at(pos); }
    TxtDataVec(int nFields_);
private:
    // virtuals from TxtData
    int  ReadLine(const char *pcLine);
    void Cur2Last(bool sum);
    
    int nFields;
    std::vector<T> cur, last;
};

template <class T>
TxtDataVec<T>::TxtDataVec(int nFields_) : nFields(nFields_)
{
    verify(nFields > 0);
    cur .resize((std::size_t)nFields);
    last.resize((std::size_t)nFields);
}

template <class T>
int TxtDataVec<T>::ReadLine(const char *pcLine)
{
    bool invalid = false;
    int iField = 0;
    for (const char *pcWord = strchr(pcLine, '\t'); pcWord != NULL; pcWord = strchr(pcWord, '\t'))
    {
        if (iField < nFields)
        {
            while (*pcWord == '\t') pcWord++;
            std::stringstream ss(pcWord);
            ss >> cur[(std::size_t)(iField++)];
            if (ss.fail() == true)
            {
                invalid = true;
                break;
            }
        }
        else
            break;
    }
    if (iField != nFields) invalid = true;
    return (invalid == false ? 0 : -1);
}

template <class T>
void TxtDataVec<T>::Cur2Last(bool sum)
{
    last = cur;
}

}

#endif /* SIBYL_SERVER_SIMULATION_TXTDATA_H_ */