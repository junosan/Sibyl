
#include "TR.h"
#include "Kiwoom_data.h"

namespace sibyl
{

// static
DispPrefix TR::dispPrefix;

TR::State TR::Send()
{
    static const long err_none = static_cast<long>(ErrorCode::OP_ERR_NONE);
    static const long err_sise = static_cast<long>(ErrorCode::OP_ERR_SISE_OVERFLOW);
    static const long err_ord  = static_cast<long>(ErrorCode::OP_ERR_ORD_OVERFLOW);
    
    State state(State::normal);
    do {
        long ret = err_none;
        do {
            ;
            // ret = SendOnce()
        } while (ret == err_sise || ret == err_ord);
        
        if (ret == err_none)
            ;// state = Wait()
    } while (IsContinuable() == true && state == State::carry);
    return state;
}

void TR::Receive(long cnt, State state)
{
    
}

void TR::Init()
{
    
}

TR::State TR::Wait(int t_timeout)
{
    State state(State::normal);
    return state;
}

void TR::End(TR::State state)
{
    
}

}
