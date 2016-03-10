#ifndef SIBYL_SERVER_TXTDATA_H_
#define SIBYL_SERVER_TXTDATA_H_

#include <iostream>
#include <vector>
#include <sstream>

#include "../Participant.h"
#include "../Security.h"

namespace sibyl
{

class TxtData
{
public:
    bool open(CSTR &filename_);
    bool is_open() const;
    void AdvanceTime(int timeTarget); // TxtDataTr requires InitSum | InitVecTr prior to this
    void SetDelay(int d);
    
    TxtData() : pf(nullptr), open_bool(false), time(TimeBounds::null), delay(0) {}
    ~TxtData() { if (pf != nullptr) fclose(pf); }
protected:
    virtual int  ReadLine(const char *pcLine) = 0; // returns non-0 to signal invalid format
    virtual void Cur2Last(bool sum)           = 0; // backup 'cur' to 'last' & and sum last (if applicable)
private:
    void AdvanceLine(); // read new line to 'cur', read time, check eof & formatting error
    static int Txt2Time(int txt);
    FILE *pf;
    STR filename;
    bool open_bool;
    int time, delay;
    constexpr static const std::size_t szBuf = (1 << 12);
    static char bufLine[szBuf];
};

char TxtData::bufLine[TxtData::szBuf];

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
    while ((open_bool == true) && (time < timeTarget)) {
        Cur2Last(true);
        AdvanceLine();
    }
}

void TxtData::AdvanceLine()
{
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
    assert(d >= 0);
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

class TxtDataTr : public TxtData
{
public:
    void InitSum  () { sumQ = sumPQ = 0; } // every kTimeTickSec sec
    void InitVecTr() { vecTr.clear();    } // every      1       sec
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

int TxtDataTr::ReadLine(const char *pcLine)
{
    int temp;
    bool invalid = false;
    if (5 == sscanf(pcLine, "%d %d %d %d %d", &temp, &cur.q, &cur.p, &cur.ps1, &cur.pb1))
    {
        cur.q   = std::abs(cur.q);
        cur.p   = std::abs(cur.p);
        cur.ps1 = std::abs(cur.ps1);
        cur.pb1 = std::abs(cur.pb1);
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
}


    /* =========================================== */
    /*                  TxtDataTb                  */
    /* =========================================== */

class TxtDataTb : public TxtData
{
public:
    const std::array<PQ, szTb>& Tb() { return last; }
    TxtDataTb(SecType type_) : type(type_) {}
private:
    // virtuals from TxtData
    int  ReadLine(const char *pcLine);
    void Cur2Last(bool sum);

    SecType type;

    std::array<PQ, szTb> cur, last;
};

int TxtDataTb::ReadLine(const char *pcLine)
{
    int temp;
    bool invalid = false;
    if ( ((type == kSecKOSPI) &&
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
         ((type == kSecELW) &&
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
    assert(nFields > 0);
    cur .resize((std::size_t)nFields);
    last.resize((std::size_t)nFields);
}

template <class T>
int TxtDataVec<T>::ReadLine(const char *pcLine)
{
    bool invalid = false;
    std::size_t iField = 0;
    for (char *pcWord = strchr(pcLine, '\t'); pcWord != NULL; pcWord = strchr(pcWord, '\t'))
    {
        if (iField < nFields)
        {
            while (*pcWord == '\t') pcWord++;
            std::stringstream ss(pcWord);
            ss >> cur[iField++];
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

#endif /* SIBYL_SERVER_TXTDATA_H_ */