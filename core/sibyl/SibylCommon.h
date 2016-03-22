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
/*     Security<Order>^ - Item^ (application specific) - KOSPI, ELW               */
/*     Catalog<Item> - Portfolio (client side data management)                    */
/*                   - OrderBook (server side data management)                    */
/*     Model^ - RewardModel                                                       */
/*     Participant^ - Trader  (Portfolio and Model interface)                     */
/*                  - Broker^ (OrderBook interface) - Simulation                  */
/*                                                  - Kiwoom                      */
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
typedef int64_t     INT64; // for price/quants that may exceed 32 bits
typedef std::string STR;   // for readability
typedef const STR   CSTR;  // for readability

// TODO: handle OS-dependency here //

static const int kTimeTickSec = 10;
static const int kReqNPerTick = 50;

static const STR kTCPPassword = "sendorder";
static const int kTCPBacklog  = 8;
static const int kTCPBufSize  = (1 << 16);

static const int szTck        = 10;        // largest tick 
static const int szTb         = szTck * 2;
static const int idxPs1       = szTck - 1;
static const int idxPb1       = szTck;
static const int idxTckOrigS0 = szTb + 0;  // not to be used on tbr
static const int idxTckOrigB0 = szTb + 1;  // not to be used on tbr

enum OrdType {   kOrdNull,
                 kOrdBuy,
                 kOrdSell          };
enum SecType {   kSecNull,
                 kSecKOSPI,
                 kSecELW           };
enum OptType {   kOptNull     =  0,
                 kOptCall     = +1,   // value specified in comm_format.txt
                 kOptPut      = -1 }; // value specified in comm_format.txt
enum ReqType {   kReqNull     = 0,
                 kReq_b       = 1,    // value specified in Kiwoom API
                 kReq_s       = 2,    // value specified in Kiwoom API
                 kReq_cb      = 3,    // value specified in Kiwoom API
                 kReq_cs      = 4,    // value specified in Kiwoom API
                 kReq_mb      = 5,    // value specified in Kiwoom API
                 kReq_ms      = 6,    // value specified in Kiwoom API
                 kReq_ca         ,
                 kReq_sa           };

class PQ
{
public:
    INT p;
    INT q;
    PQ()               : p(0 ), q(0 ) {}
    PQ(int p_, int q_) : p(p_), q(q_) {}
};

#ifdef NDEBUG
    #define verify(expression) ((void)(expression))
#else
    #include <cassert>
    #define verify(expression) assert(expression)
#endif /* NDEBUG */

}

#endif /* SIBYL_COMMON_H_ */