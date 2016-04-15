#ifndef SIBYL_COMMON_H_
#define SIBYL_COMMON_H_

/* ============================================================================== */
/*         SIBYL: Simulation of Intraday Book and Yield with Limit-orders         */
/* ============================================================================== */

/* ============================================================================== */
/*                                Inheritance tree                                */
/* ------------------------------------------------------------------------------ */ 
/* (^: abstract class)                                                            */
/*     PQ - Order (application specific)                                          */
/*     Security<Order>^ - Item^ (application specific) - KOSPI, ELW, ETF          */
/*     Catalog<Item> - Portfolio (client side data management)                    */
/*                   - OrderBook (server side data management)                    */
/*     Model^ - RewardModel                                                       */
/*     Trader  (Portfolio and Model interface)                                    */
/*     Broker^ (OrderBook interface) - Simulation, Kiwoom                         */
/*     NetAgent - NetClient (TCP client and &Trader interface)                    */
/*              - NetServer (TCP server and &Broker interface)                    */
/*     TxtData^ - TxtDataTr, TxtDataTb, TxtDataVec                                */
/* ============================================================================== */

/* ============================================================================== */
/*                                  Terminology                                   */
/* ------------------------------------------------------------------------------ */
/*    ord : buy/sell (a.k.a. bid/ask) registered in Security by my request        */
/*    req : request for action from client to server (b/s/cb/cs/mb/ms/etc.)       */
/*    msg : content of TCP communication                                          */
/* ============================================================================== */

#include <string>

namespace sibyl
{

typedef float       FLOAT; // for floats less than 7 decimal significant digits
typedef int         INT;   // for price/quants that are not expected to exceed 32 bits
typedef int64_t     INT64; // for price/quants that may exceed 32 bits in extreme cases
typedef std::string STR;   // for brevity
typedef const STR   CSTR;  // for brevity

#ifdef NDEBUG
    #define verify(expression) ((void)(expression))
    #if defined _WIN32 && !defined DISABLE_VERIFY_IN_RELEASE_MODE
        #undef verify
        #include <iostream>
        #include <errhandlingapi.h>
        #include <stdlib.h>
        #include <string.h>
        #define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
        #define verify(expression) ((expression) == false && (std::cerr << "ERROR: " << __FILENAME__ << ", line " << __LINE__ << ", function " << __func__ << std::endl, SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX), abort(), false))
    #endif /* _WIN32 && !DISABLE_VERIFY_IN_RELEASE_MODE */
#else
    #include <cassert>
    #define verify(expression) assert(expression)
#endif /* NDEBUG */

}

#endif /* SIBYL_COMMON_H_ */