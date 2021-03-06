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

#ifndef SIBYL_SERVER_ORDERBOOK_DATA_H_
#define SIBYL_SERVER_ORDERBOOK_DATA_H_

#include <map>

#include "../Security.h"
#include "../ReqType.h"

namespace sibyl
{

class Order : public PQ
{
public:
    OrdType type;
    int     tck_orig; // to be automatically filled by ApplyInsert function
    Order()             :           type(OrdType::null), tck_orig(idx::tckN) {}
    Order(INT p, INT q) : PQ(p, q), type(OrdType::null), tck_orig(idx::tckN) {}
};

template <class TOrder>
class Item : public Security<TOrder>
{
public:
    virtual ~Item() {}
};

// shorthand for Catalog.items's iterator
template <class TItem>
using it_itm_t = typename std::map<STR, std::unique_ptr<TItem>>::iterator;

template <class TItem>
class UnnamedReq
{
public:
    ReqType         type;
    it_itm_t<TItem> iItems;
    INT             p;
    INT             q;
    INT             mp; // only for mb, ms
    UnnamedReq() : type(ReqType::null), p(0), q(0), mp(0) {}
};

// shorthand for Security.ord's iterator
template <class TOrder>
using it_ord_t = typename std::multimap<INT, TOrder>::iterator;

template <class TOrder, class TItem>
class NamedReq
{
public:
    ReqType          type;
    it_itm_t<TItem>  iItems;
    it_ord_t<TOrder> iOrd; // only for cb, cs, mb, ms
    INT              p;    // only for b, s, mb, ms (p = mp for mb|ms)
    INT              q;
    NamedReq() : type(ReqType::null), p(0), q(0) {}
};

}

#endif /* SIBYL_SERVER_ORDERBOOK_DATA_H_ */