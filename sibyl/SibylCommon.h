#ifndef SIBYL_COMMON_H_
#define SIBYL_COMMON_H_

/* ============================================================================== */
/*                                Inheritance tree                                */
/* ============================================================================== */ 
/* (^: abstract class)                                                            */
/*     PQ - Order (application specific)                                          */
/*     Security<container<Order>>^ - Item^ (application specific) - Stock         */
/*                                                                - ELW           */
/*     Catalog<Item> - Portfolio (client side data management)                    */
/*                   - OrderBook (server side data management) - Simulation       */
/*                                                             - OrderBookKiwoom  */
/*     NetClient (wraps TCP client and interfaces with &Portfolio)                */
/*     NetServer (wraps TCP server and interfaces with &Orderbook)                */
/* ============================================================================== */
/*                                  Terminology                                   */
/* ============================================================================== */
/*    ord : bid/ask registered in Security by my request                          */
/*    req : request for action from client to server                              */
/*    msg : content of TCP communication                                          */
/* ============================================================================== */
    
#include <string>

namespace sibyl
{

typedef float   FLOAT; // for floats less than 7 decimal significant digits
typedef int     INT;   // for price/quants that are not expected to exceed 32 bits
typedef int64_t INT64; // for price/quants that may exceed 32 bits

// TODO: handle OS-dependency here //

static const int         kTimeTickSec = 10;
static const int         kReqNPerTick = 50;

static const std::string kTCPPassword = "sendorder";
static const int         kTCPBacklog  = 8;
static const int         kTCPBufSize  = (1 << 16);

static const int         szTck        = 10;        // largest tick 
static const int         szTb         = szTck * 2;
static const int         idxPs1       = szTck - 1;
static const int         idxPb1       = szTck;
static const int         idxPs0       = szTb + 0;  // not to be used on tbr
static const int         idxPb0       = szTb + 1;  // not to be used on tbr

enum OrdType {           kOrdNull,
                         kOrdBuy,
                         kOrdSell          };
enum SecType {           kSecStock,
                         kSecELW           };
enum OptType {           kOptNull     =  0,
                         kOptCall     = +1,   // value specified in comm_format.txt
                         kOptPut      = -1 }; // value specified in comm_format.txt
enum ReqType {           kReqNull     = 0,
                         kReq_b       = 1,    // value specified in Kiwoom API
                         kReq_s       = 2,    // value specified in Kiwoom API
                         kReq_cb      = 3,    // value specified in Kiwoom API
                         kReq_cs      = 4,    // value specified in Kiwoom API
                         kReq_mb      = 5,    // value specified in Kiwoom API
                         kReq_ms      = 6,    // value specified in Kiwoom API
                         kReq_ca         ,
                         kReq_sa           };

}

#endif /* SIBYL_COMMON_H_ */