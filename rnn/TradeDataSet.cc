#include "TradeDataSet.h"

#include <cstring>
#include <fstream>
#include <cstdint>
#include <cmath>
#include <iostream>

using namespace fractal;

const unsigned long TradeDataSet::CHANNEL_INPUT = 0;
const unsigned long TradeDataSet::CHANNEL_TARGET = 1;
const unsigned long TradeDataSet::CHANNEL_SIG_NEWSEQ = 2;

TradeDataSet::TradeDataSet() : reshaper(1) // reshaper(maxGTck)
{
    nSeq = 0;

    inputDim  = reshaper.GetInputDim();
    targetDim = reshaper.GetTargetDim();
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
            fileList[i] = *it;
        }
        else /* Relative path */
        {
            pos = filename.find_last_of('/');
            if(pos != std::string::npos)
            {
                fileList[i] = filename.substr(0, pos + 1) + *it;
            }
            else
            {
                fileList[i] = *it;
            }
        }
    }

    return fileList.size();
}


void TradeDataSet::ReadData()
{
    unsigned long i;

    nSeq = fileList.size();

    nFrame.clear();
    input.clear();
    target.clear();

    nFrame.shrink_to_fit();
    input.shrink_to_fit();
    target.shrink_to_fit();

    nFrame.resize(nSeq);
    input.resize(nSeq);
    target.resize(nSeq);

    for(i = 0; i < nSeq; i++)
    {
        unsigned long nRaw, nRef;

        nRaw = ReadRawFile(input[i], fileList[i] + ".raw");
        nRef = ReadRefFile(target[i], fileList[i] + ".ref");

        verify(nRaw == nRef);
        nFrame[i] = nRaw;
    }
}


void TradeDataSet::Normalize()
{
    unsigned long i, j, k, n;
    FLOAT val;
    std::vector<double> sum, ssum;

    mean.resize(inputDim);
    mean.shrink_to_fit();

    stdev.resize(inputDim);
    stdev.shrink_to_fit();

    sum.assign(inputDim, 0.0);
    ssum.assign(inputDim, 0.0);

    n = 0;

    for(i = 0; i < nSeq; i++)
    {
        n += nFrame[i];
        for(j = 0; j < nFrame[i]; j++)
        {
            for(k = 0; k < inputDim; k++)
            {
                val = input[i][j * inputDim + k];
                sum[k] += val;
                ssum[k] += val * val;
            }
        }
    }

    for(k = 0; k < inputDim; k++)
    {
        mean[k] = sum[k] / n;
        stdev[k] = sqrt((ssum[k] / n) - mean[k] * mean[k]);
    }

    Normalize(mean, stdev);
}


void TradeDataSet::Normalize(const std::vector<FLOAT> &mean, const std::vector<FLOAT> &stdev)
{
    unsigned long i, j, k;

    if(&this->mean != &mean)
        this->mean = mean;

    if(&this->stdev != &stdev)
        this->stdev = stdev;

    for(i = 0; i < nSeq; i++)
    {
        for(j = 0; j < nFrame[i]; j++)
        {
            for(k = 0; k < inputDim; k++)
            {
                input[i][j * inputDim + k] = (input[i][j * inputDim + k] - mean[k]) / stdev[k];
            }
        }
    }
}


const std::vector<FLOAT> &TradeDataSet::GetMean()
{
    return mean;
}


const std::vector<FLOAT> &TradeDataSet::GetStdev()
{
    return stdev;
}


const unsigned long TradeDataSet::GetNumChannel() const
{
    return 3;
}


const ChannelInfo TradeDataSet::GetChannelInfo(const unsigned long channelIdx) const
{
    ChannelInfo channelInfo;

    switch(channelIdx)
    {
        case CHANNEL_INPUT:
            channelInfo.dataType = ChannelInfo::DATATYPE_VECTOR;
            channelInfo.frameSize = inputDim;
            channelInfo.frameDim = inputDim;
            break;

        case CHANNEL_TARGET:
            channelInfo.dataType = ChannelInfo::DATATYPE_VECTOR;
            channelInfo.frameSize = targetDim;
            channelInfo.frameDim = targetDim;
            break;

        case CHANNEL_SIG_NEWSEQ:
            channelInfo.dataType = ChannelInfo::DATATYPE_VECTOR;
            channelInfo.frameSize = 1;
            channelInfo.frameDim = 1;
            break;

        default:
            verify(false);
    }

    return channelInfo;
}


const unsigned long TradeDataSet::GetNumSeq() const
{
    return nSeq;
}


const unsigned long TradeDataSet::GetNumFrame(const unsigned long seqIdx) const
{
    verify(seqIdx < nSeq);

    return nFrame[seqIdx];
}


void TradeDataSet::GetFrameData(const unsigned long seqIdx, const unsigned long channelIdx,
        const unsigned long frameIdx, void *const frame)
{
    verify(seqIdx < nSeq);
    verify(frameIdx < nFrame[seqIdx]);

    switch(channelIdx)
    {
        case CHANNEL_INPUT:
            memcpy(frame, input[seqIdx].data() + inputDim * frameIdx, sizeof(FLOAT) * inputDim);
            break;

        case CHANNEL_TARGET:
            memcpy(frame, target[seqIdx].data() + targetDim * frameIdx, sizeof(FLOAT) * targetDim);
            break;

        case CHANNEL_SIG_NEWSEQ:
            reinterpret_cast<FLOAT *>(frame)[0] = (FLOAT) (frameIdx == 0);
            break;

        default:
            verify(false);
    }
}


const unsigned long TradeDataSet::ReadRawFile(std::vector<fractal::FLOAT> &vec, const std::string &filename)
{
    /*
       % code.raw
       % little endian 32bit signed int (pr is float32)
       % (43 fields) * (T entries)
       t pr qr tbpr(1:20) tbqr(1:20) (high->low price ordering)
    */

    const long interval = 10; // seconds
    const long rawDim = 43;
    const long T = std::ceil((6 * 3600 - 10 * 60)/interval) - 1;
    const long n = inputDim * T;
    const long nRaw = rawDim * T;

    vec.resize(n);

    /* Read code */
    auto posSlash = filename.find_last_of('/');
    auto posDot   = filename.find_last_of('.');
    auto code     = filename.substr(posSlash + 1, posDot - posSlash - 1);

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
    std::vector<Data32> buf32(n);
#endif

    /* Allocate memory */
    std::vector<Data32> buf32(n);

    /* Read features */
    fseek(f, 0, SEEK_SET);
    fread(buf32.data(), sizeof(Data32), nRaw, f);
    fclose(f);

    sibyl::ItemState state;
    state.code = code;

    for(long t = 0; t < T; t++)
    {
        long idxInput = t * inputDim;
        long idxRaw = t * rawDim;

        // t
        data32.uint32 = le32toh(buf32[idxRaw + 0].uint32);
        state.time = (int) data32.int32;

        // pr
        data32.uint32 = le32toh(buf32[idxRaw + 1].uint32);
        state.pr = (sibyl::FLOAT) data32.float32;

        // qr
        data32.uint32 = le32toh(buf32[idxRaw + 2].uint32);
        state.qr = (sibyl::INT64) data32.int32;

        // tbpr(1:20)
        for(long i = 0; i < (long) sibyl::szTb; i++)
        {
            data32.uint32 = le32toh(buf32[idxRaw + 3 + i].uint32);
            state.tbr[i].p = (sibyl::INT) data32.int32;
        }

        // tbqr(1:20)
        for(long i = 0; i < (long) sibyl::szTb; i++)
        {
            data32.uint32 = le32toh(buf32[idxRaw + 23 + i].uint32);
            state.tbr[i].q = (sibyl::INT) data32.int32;
        }
        
        /* Write on vec based on state */ 
        reshaper.State2Vec(vec.data() + idxInput, state);
    }

    for(long i = 0; i < n; i++)
    {
        if(isinff(vec[i]) || isnanf(vec[i]) || isinf(vec[i]) || isnan(vec[i]))
        {
            std::cerr << "ERR: " << filename << " (" << i / inputDim << ", " << i % inputDim << ") " << vec[i] << std::endl;
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

    const long interval = 10; // seconds
    const long refDim = 42;
    const long T = std::ceil((6 * 3600 - 10 * 60)/interval) - 1;
    const long n = targetDim * T;
    const long nRef = refDim * T;

    vec.resize(n);

    /* Read code */
    auto posSlash = filename.find_last_of('/');
    auto posDot   = filename.find_last_of('.');
    auto code     = filename.substr(posSlash + 1, posDot - posSlash - 1);

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
    fread(buf32.data(), sizeof(Data32), nRef, f);
    fclose(f);

    sibyl::Reward reward;
    long maxGTck = (long) reshaper.GetMaxGTck();

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
        reshaper.Reward2Vec(vec.data() + t * targetDim, reward, code);
    }

    for(long i = 0; i < n; i++)
    {
        //if(i % targetDim == 0) std::cout << std::endl;
        //std::cout << vec[i] << " ";
        if(isinff(vec[i]) || isnanf(vec[i]) || isinf(vec[i]) || isnan(vec[i]))
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

