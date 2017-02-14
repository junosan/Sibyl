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

#ifndef SIBYL_CLIENT_REWARDMODEL_H_
#define SIBYL_CLIENT_REWARDMODEL_H_

#include <vector>
#include <map>
#include <fstream>
#include <cstring>

#include "../Security.h"
#include "Portfolio.h"
#include "Model.h"
#include "Reward.h"

namespace sibyl
{

class RewardModel : public Model
{
public:  
    void SetParams(double timeConst_, double rhoWeight_, double rhoInit_,
                   bool exclusiveBuy_, bool sellBeforeEnd_, bool earlyQuit_,
                   bool patientB0_, bool patientS0_);
    void ReadConfig(CSTR &filename);

    // for ref
    void SetRefPath(CSTR &path);
    void GetRefData();
    
    // for rnn 
    std::vector<Reward>& GetRewardVec(); // returns vec with correct size (0-filled)
    void                 SetRewardVec(const std::vector<Reward> &vec); // put in here after filling content

    // virtuals from Model; to be called by Trader
    void  SetStateLogPaths(CSTR &state, CSTR &log);
    CSTR& BuildMsgOut     ();
    
    RewardModel() : timeConst(0.0), rhoWeight(0.0), rho(0.0),
                    exclusiveBuy(false), sellBeforeEnd(false), earlyQuit(false),
                    patientB0(false), patientS0(false),
                    exitMarket(false), idx_rate_r(0), isFirstTick(true) {}
private:
    // parameters
    double timeConst, rhoWeight, rho;
    bool   exclusiveBuy, sellBeforeEnd, earlyQuit, patientB0, patientS0;

    // early quit mechanism
    bool exitMarket;
    std::vector<double> vec_rate_r;
    std::size_t         idx_rate_r;

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
        if (kTimeRates::reqPerTick == ++nMsgCurTick)
            return true;
        return false;        
    }
    STR msg;
};

}

#endif /* SIBYL_CLIENT_REWARDMODEL_H_ */