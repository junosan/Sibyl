
#include <thread>
#include <chrono>
#include <vector>

#include "TR.h"
#include "Kiwoom_data.h"
#include "../../util/Clock.h"
#include "../../util/DispPrefix.h"

namespace sibyl
{

// static
std::map<STR, TR*> TR::map_name_TR;

TR::State TR::Send(bool write)
{
    writeOrderBook = write;
    
    State state(State::normal);
    do {
        long ret;
        while (true)
        {
            Init();
            ret = SendOnce(state);
            if (ret == kKiwoomError::OP_ERR_SISE_OVERFLOW ||
                ret == kKiwoomError::OP_ERR_ORD_OVERFLOW   )
            {
                std::cerr << dispPrefix << Name() << "::Send: Rate overflow" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(t_waitOverflow));
            }
            else
                break;
        }
        
        if (ret == kKiwoomError::OP_ERR_NONE)
            state = Wait(AllowTimeout() == true ? t_timeout : kNoTimeout);
        else
            std::cerr << dispPrefix << Name() << "::Send: Error " << static_cast<long>(ret) << std::endl;
    } while (AllowCarry() == true && state == State::carry);
 
    return state;
}

// static
void TR::Receive(CSTR &name, long cnt, State state)
{
    auto it = map_name_TR.find(name);
    if (it != std::end(map_name_TR))
    {
        it->second->RetrieveData(cnt);
        it->second->End(state);
    }
    else
        std::cerr << dispPrefix << "TR::Receive: Unknown name " << name << std::endl;    
}

void TR::Register()
{
    auto it_success = map_name_TR.insert(std::make_pair(Name(), this));
    if (it_success.second == false)
    {
        std::cerr << dispPrefix << Name() << "::Register: Name must be unique for each instance" << std::endl;
        verify(false);
    }
}

void TR::Deregister()
{
    auto it = map_name_TR.find(Name());
    if (it != std::end(map_name_TR)) map_name_TR.erase(it);
}

void TR::Init()
{
    static std::vector<int> t;
    static std::size_t cur = 0; // cursor to last kTimeRates::reqPerSec-th time this function was called
    
    if (kTimeRates::reqPerSec > 0)
    {
        t.resize(kTimeRates::reqPerSec, kTimeBounds::null);
        int t_target = t[cur] + 1000;
        int t_now;
        while (true)
        {
            t_now = clock.Now();
            if (t_now <= t_target)
                std::this_thread::sleep_for(std::chrono::milliseconds(t_target - t_now));
            else
                break;
        }
        t[cur] = t_now;
        cur = (cur + 1) % kTimeRates::reqPerSec;
    }
    
    end_bool = false;
}

TR::State TR::Wait(int t_timeout)
{
    State state(State::normal);
    
    std::unique_lock<std::mutex> lock(cv_mutex);
    if (t_timeout >= 0)
    {
        if (cv.wait_for(lock, std::chrono::milliseconds(t_timeout), [&]{ return end_bool == true; })) 
            state = end_state;
        else
            state = State::timeout;
    }
    else if (t_timeout == kNoTimeout)
    {
        cv.wait(lock, [&]{ return end_bool == true; });
        state = end_state;
    }
    else
        verify(false);
    
    return state;
}

void TR::End(TR::State state)
{
    end_state = state;
    end_bool = true;
    cv.notify_one();
}

}
