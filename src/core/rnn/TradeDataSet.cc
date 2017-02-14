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

#include "TradeDataSet.h"
#include <sibyl/server/Simulation/Simulation_dep.h>

#include <cstring>
#include <fstream>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <set>

using namespace fractal;

TradeDataSet::TradeDataSet() : nSeq(0),
                               pReshaper(nullptr),
                               inputDim(0),
                               targetDim(0)
{

}


const unsigned long TradeDataSet::ReadFileList(const std::string &filename)
{
    /* Read list file */
    std::list<std::string> tmpList;
    std::ifstream listFile;
    std::string buf;
    size_t pos1, pos2;

    listFile.open(filename, std::ios_base::in);

    verify(listFile.is_open() == true);

    while(true)
    {
        std::getline(listFile, buf);

        if(listFile.eof() == true) break;
        verify(listFile.good() == true);

        pos1 = buf.find_first_not_of(" \n\r\t");

        if(pos1 == std::string::npos) continue;
        pos2 = buf.find_last_not_of(" \n\r\t");

        if(pos2 >= pos1)
            tmpList.push_back(buf.substr(pos1, pos2 - pos1 + 1));
    }

    listFile.close();

    fileList.clear();
    fileList.shrink_to_fit();
    fileList.resize(tmpList.size());

    /* Fix relative path */
    size_t pos;
    unsigned long i = 0;

    for(auto it = tmpList.begin(); it != tmpList.end(); it++, i++)
    {
        if((*it)[0] == '/') /* Absolute path */
        {
            fileList[i] = *it; // use as is
        }
        else /* Relative path */
        {
            pos = filename.find_last_of('/');
            if(pos != std::string::npos) // .list file in a different relative path
            {
                // get .list file's path and append data file 
                fileList[i] = filename.substr(0, pos + 1) + *it;
            }
            else // .list file in pwd
            {
                fileList[i] = *it;
            }
        }
    }

    return (nSeq = fileList.size());
}

void TradeDataSet::ReadData(bool verbose)
{
    input.clear();
    target.clear();

    input.shrink_to_fit();
    target.shrink_to_fit();

    input.resize(nSeq);
    target.resize(nSeq);

    const unsigned long tenth = nSeq / 10;

    for(auto i = 0u; i < nSeq; ++i)
    {
        if (verbose == true && i % tenth == 0 && i > 0)
            std::cerr << i / tenth << "0% ";

        unsigned long nRaw, nRef;

        nRaw = ReadRawFile(input[i], fileList[i] + ".raw");
        nRef = ReadRefFile(target[i], fileList[i] + ".ref");

        verify(nRaw == nRef);
    }
}

void TradeDataSet::DumpData(const std::string &path, bool verbose)
{
    std::set<std::string> dates_seen;

    const unsigned long tenth = nSeq / 10;

    for (auto i = 0u; i < nSeq; ++i)
    {
        if (verbose == true && i % tenth == 0 && i > 0)
            std::cerr << i / tenth << "0% ";

        std::string date = fileList[i].substr(0, fileList[i].find_last_of('/'));
        date = date.substr(date.find_last_of('/') + 1);
        std::string code = fileList[i].substr(fileList[i].find_last_of('/') + 1);
        std::string dir = path + '/' + date;

        {
            bool new_date(false);
            std::tie(std::ignore, new_date) = dates_seen.insert(date);
            if (new_date == true)
                verify(0 == system(std::string("mkdir -p " + dir).c_str()));
        }

        {
            std::string filename  = dir + '/' + code + ".input";
            std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);
            out.write((char*) input[i].data(), input[i].size() * sizeof(fractal::FLOAT));
        }

        {
            std::string filename  = dir + '/' + code + ".target";
            std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);
            out.write((char*) target[i].data(), target[i].size() * sizeof(fractal::FLOAT));
        }
        
    }
}

const unsigned long TradeDataSet::GetNumChannel() const
{
    return 3;
}

const unsigned long TradeDataSet::GetNumSeq() const
{
    return nSeq;
}


const unsigned long TradeDataSet::GetNumFrame(const unsigned long seqIdx) const
{
    verify(seqIdx < nSeq);

    return nFrame;
}

void TradeDataSet::GetFrameData(const unsigned long seqIdx, const unsigned long channelIdx,
        const unsigned long frameIdx, void *const frame)
{
    verify(seqIdx < nSeq);
    verify(frameIdx < nFrame);

    const FLOAT *data(nullptr);
    const FLOAT isFirstFrame = (FLOAT) (frameIdx == 0);

    switch(channelIdx)
    {
        case CHANNEL_INPUT:
            data = input[seqIdx].data() + inputDim * frameIdx;
            break;

        case CHANNEL_TARGET:
            data = target[seqIdx].data() + targetDim * frameIdx;
            break;

        case CHANNEL_SIG_NEWSEQ:
            data = &isFirstFrame;
            break;

        default:
            verify(false);
    }

    PutFrameData(data, channelIdx, frame);
}

const unsigned long TradeDataSet::ReadRawFile(std::vector<fractal::FLOAT> &vec, const std::string &filename)
{
    /*
        % code.raw
        % little endian 32bit signed int (pr is float32)
        % (43 fields) * (T entries)
        t pr qr tbpr(1:20) tbqr(1:20) (high->low price ordering)

        % $code.thg (ELW only)
        % little endian 32bit float
        % (8 fields) * (T entries)
        [ theoretical price
        inherent volatility
        delta
        gamma
        theta
        vega
        rho
        LP inherent volatility ]
        
        % kospi200.idx (ELW only)
        % little endian 32bit float
        % (1 field) * (T entries)
        [ KOSPI200_index (or equivalent) ]
    */
    
    FILE *f;
    
    union Data32
    {
        uint32_t uint32;
        int32_t int32;
        float float32;
    } data32;
    
    sibyl::ItemState state;
    
    /* Read code from filename */
    auto posSlash  = filename.find_last_of('/');
    auto posDot    = filename.find_last_of('.');
    auto path      = filename.substr(0, posSlash + 1); // includes '/'
    auto posSlash2 = filename.substr(0, posSlash).find_last_of('/'); 
    auto date      = filename.substr(posSlash2 + 1, posSlash - posSlash2 - 1);
    auto code      = filename.substr(posSlash + 1, posDot - posSlash - 1);
    
    verify(date.empty() == false && code.empty() == false);
    
    state.code = date + "/" + code;

#if 0
    /* Read the number of samples */
    fseek(f, 0, SEEK_SET);
    fread(&data32, sizeof(Data32), 1, f);
    data32.uint32 = be32toh(data32.uint32);
    nFrame[seqIdx] = data32.uint32;

    /* Read the dimension of samples */
    fseek(f, 8, SEEK_SET);
    fread(&data16, sizeof(Data16), 1, f);
    data16.uint16 = be16toh(data16.uint16);
    n = data16.uint16 >> 2;
    if(featDim == 0) featDim = n;
    else verify(featDim == n);

    /* Allocate memory */
    n = nFrame[seqIdx] * featDim;
    feature[seqIdx].resize(n);
    std::vector<Data32> buf32_raw(n);
#endif

    /* Number of frames */
    const long T = nFrame;
    
    /* .raw file */
    const long rawDim = 43;
    const long nRaw = rawDim * T;
    std::vector<Data32> buf32_raw(nRaw);
    
    f = fopen(filename.c_str(), "rb");
    verify(f != nullptr);
    fseek(f, 0, SEEK_SET);
    auto szRead = fread(buf32_raw.data(), sizeof(Data32), nRaw, f);
    verify(szRead > 0);
    fclose(f);
    
    /* Identify ELW */
    bool isELW(false);
    auto infoFile = path + code + "i.txt";
    f = fopen(infoFile.c_str(), "r");
    if (f != nullptr)
    {
        fclose(f);
        isELW = true;
        
        int te = sibyl::Simulation::ReadTypeExpiry(path, code);
        verify(te != 0);
        
        state.isELW  = true;
        state.iCP    = (te > 0) - (te < 0); // +1 for call, -1 for put
        state.expiry = std::abs(te);
    }
    
    /* .thg file (ELW only) */
    const long thgDim = 8;
    const long nThg = thgDim * T;
    std::vector<Data32> buf32_thg; // resized below
    
    if (isELW == true)
    {
        buf32_thg.resize(nThg);
        auto thgFile = path + code + ".thg";
        f = fopen(thgFile.c_str(), "rb");
        verify(f != nullptr);
        fseek(f, 0, SEEK_SET);
        auto szRead = fread(buf32_thg.data(), sizeof(Data32), nThg, f);
        verify(szRead > 0);
        fclose(f);
    }
    
    /* .idx file (ELW only) */
    const long idxDim = 1;
    const long nIdx = idxDim * T;
    std::vector<Data32> buf32_idx; // resized below
    
    if (isELW == true)
    {
        buf32_idx.resize(nIdx);
        auto idxFile = path + "kospi200.idx";
        f = fopen(idxFile.c_str(), "rb");
        verify(f != nullptr);
        fseek(f, 0, SEEK_SET);
        auto szRead = fread(buf32_idx.data(), sizeof(Data32), nIdx, f);
        verify(szRead > 0);
        fclose(f);
    }

    /* Prepare vec */
    const long n = inputDim * T;
    vec.resize(n);

    for(long t = 0; t < T; t++)
    {
        /* .raw file frame data */
        long idxRaw = t * rawDim;

        // t
        data32.uint32 = le32toh(buf32_raw[idxRaw + 0].uint32);
        state.time = (int) data32.int32;

        // pr
        data32.uint32 = le32toh(buf32_raw[idxRaw + 1].uint32);
        state.pr = (sibyl::FLOAT) data32.float32;

        // qr
        data32.uint32 = le32toh(buf32_raw[idxRaw + 2].uint32);
        state.qr = (sibyl::INT64) data32.int32;

        // tbpr(1:20)
        for(long i = 0; i < (long) sibyl::idx::szTb; i++)
        {
            data32.uint32 = le32toh(buf32_raw[idxRaw + 3 + i].uint32);
            state.tbr[i].p = (sibyl::INT) data32.int32;
        }

        // tbqr(1:20)
        for(long i = 0; i < (long) sibyl::idx::szTb; i++)
        {
            data32.uint32 = le32toh(buf32_raw[idxRaw + 23 + i].uint32);
            state.tbr[i].q = (sibyl::INT) data32.int32;
        }
        
        if (isELW == true)
        {            
            /* .thg file frame data */
            long idxThg = t * thgDim;
            
            // theoretical price
            data32.uint32 = le32toh(buf32_thg[idxThg + 0].uint32);
            state.thr[0] = (sibyl::FLOAT) data32.float32;
            
            // volatility
            data32.uint32 = le32toh(buf32_thg[idxThg + 1].uint32);
            state.thr[1] = (sibyl::FLOAT) data32.float32;
            
            // delta
            data32.uint32 = le32toh(buf32_thg[idxThg + 2].uint32);
            state.thr[2] = (sibyl::FLOAT) data32.float32;
            
            // gamma
            data32.uint32 = le32toh(buf32_thg[idxThg + 3].uint32);
            state.thr[3] = (sibyl::FLOAT) data32.float32;
            
            // theta
            data32.uint32 = le32toh(buf32_thg[idxThg + 4].uint32);
            state.thr[4] = (sibyl::FLOAT) data32.float32;
            
            // vega
            data32.uint32 = le32toh(buf32_thg[idxThg + 5].uint32);
            state.thr[5] = (sibyl::FLOAT) data32.float32;
            
            // rho
            data32.uint32 = le32toh(buf32_thg[idxThg + 6].uint32);
            state.thr[6] = (sibyl::FLOAT) data32.float32;
            
            // LP volatility
            data32.uint32 = le32toh(buf32_thg[idxThg + 7].uint32);
            state.thr[7] = (sibyl::FLOAT) data32.float32;
            
            /* .idx file frame data */
            long idxIdx = t * idxDim;
            
            // kospi200
            data32.uint32 = le32toh(buf32_idx[idxIdx + 0].uint32);
            state.kospi200 = (sibyl::FLOAT) data32.float32;
        }
        
        /* Write on vec based on state */
        long idxInput = t * inputDim;
        pReshaper->State2VecIn(vec.data() + idxInput, state);
    }

    for(long i = 0; i < n; i++)
    {
        if(isinff(vec[i]) || isnanf(vec[i]) || std::isinf(vec[i]) || std::isnan(vec[i]))
        {
            std::cerr << "ERR: " << date << "/" << code << " (" << i / inputDim << ", " << i % inputDim << ") " << vec[i] << std::endl;
        }
        //verify(!isnan(vec[i]));
        //verify(!isinf(vec[i]));
        //verify(!isnanf(vec[i]));
        //verify(!isinff(vec[i]));
    }

    return T;
}


const unsigned long TradeDataSet::ReadRefFile(std::vector<fractal::FLOAT> &vec, const std::string &filename)
{
    /*
       % code.ref
       % little endian 32bit float
       % (42 fields) * (T entries)
       Gs0 Gb0 Gs(1:10) Gb(1:10) Gcs(1:10) Gcb(1:10)
    */

    const long refDim = 42;
    const long T = nFrame;
    const long n = targetDim * T;
    const long nRef = refDim * T;

    vec.resize(n);

    /* Read code from filename */
    auto posSlash  = filename.find_last_of('/');
    auto posDot    = filename.find_last_of('.');
    auto path      = filename.substr(0, posSlash + 1); // includes '/'
    auto posSlash2 = filename.substr(0, posSlash).find_last_of('/'); 
    auto date      = filename.substr(posSlash2 + 1, posSlash - posSlash2 - 1);
    auto code      = filename.substr(posSlash + 1, posDot - posSlash - 1);
    
    verify(date.empty() == false && code.empty() == false);
    
    code = date + "/" + code;

    /* Read file */
    FILE *f;

    union Data32
    {
        uint32_t uint32;
        int32_t int32;
        float float32;
    } data32;


    f = fopen(filename.c_str(), "rb");
    verify(f != NULL);

    /* Allocate memory */
    std::vector<Data32> buf32(nRef);

    /* Read features */
    fseek(f, 0, SEEK_SET);
    auto szRead = fread(buf32.data(), sizeof(Data32), nRef, f);
    verify(szRead > 0);
    fclose(f);

    sibyl::Reward reward;
    long maxGTck = (long) pReshaper->GetMaxGTck();
  
    /* (if present) rewind any time-dependent variables */
    pReshaper->Reward2VecOut(nullptr, reward, code);

    for(long t = 0; t < T; t++)
    {
        data32.uint32 = le32toh(buf32[t * refDim + 0].uint32);
        reward.G0.s = (sibyl::FLOAT) data32.float32;

        data32.uint32 = le32toh(buf32[t * refDim + 1].uint32);
        reward.G0.b = (sibyl::FLOAT) data32.float32;

        for(long j = 0; j < maxGTck; j++) {
            data32.uint32  = le32toh(buf32[t * refDim + 2 + j].uint32);
            reward.G[j].s  = (sibyl::FLOAT) data32.float32;
        }
        for(long j = 0; j < maxGTck; j++) {
            data32.uint32  = le32toh(buf32[t * refDim + 12 + j].uint32);
            reward.G[j].b  = (sibyl::FLOAT) data32.float32;
        }
        for(long j = 0; j < maxGTck; j++) {
            data32.uint32  = le32toh(buf32[t * refDim + 22 + j].uint32);
            reward.G[j].cs = (sibyl::FLOAT) data32.float32;
        }
        for(long j = 0; j < maxGTck; j++) {
            data32.uint32  = le32toh(buf32[t * refDim + 32 + j].uint32);
            reward.G[j].cb = (sibyl::FLOAT) data32.float32;
        }

        /* Write on vec based on reward */
        pReshaper->Reward2VecOut(vec.data() + t * targetDim, reward, code);
    }

    for(long i = 0; i < n; i++)
    {
        //if(i % targetDim == 0) std::cout << std::endl;
        //std::cout << vec[i] << " ";
        if(isinff(vec[i]) || isnanf(vec[i]) || std::isinf(vec[i]) || std::isnan(vec[i]))
        {
            std::cerr << "ERR: " << filename << " (" << i / targetDim << ", " << i % targetDim << ") " << vec[i] << std::endl;
        }
        //verify(!isnan(vec[i]));
        //verify(!isinf(vec[i]));
        //verify(!isnanf(vec[i]));
        //verify(!isinff(vec[i]));
    }

    return T;
}

