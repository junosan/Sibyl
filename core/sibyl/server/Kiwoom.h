#ifndef SIBYL_SERVER_KIWOOM_H_
#define SIBYL_SERVER_KIWOOM_H_

#include "Broker.h"

// use freopen to redirect stdout/stderr to files & ios::sync_with_stdio()
// use Tail for Windows to monitor (https://sourceforge.net/projects/wintail)

// or consider
// http://stackoverflow.com/questions/311955/redirecting-cout-to-a-console-in-windows

using namespace sibyl
{

class OrderKw : public Order
{
public:
    STR ordno;
};

class ItemKw : public Item<OrderKw>
{
public:
    STR srcno;
};

class Kiwoom : public Broker<OrderKw, ItemKw>
{
public:
    
};

}

#endif /* SIBYL_SERVER_KIWOOM_H_ */