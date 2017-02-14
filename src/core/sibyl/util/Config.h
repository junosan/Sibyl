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

#ifndef SIBYL_UTIL_CONFIG_H
#define SIBYL_UTIL_CONFIG_H

#include <sstream>
#include <vector>
#include <map>

#include "../sibyl_common.h"

namespace sibyl
{

// Read/write config files of format
//
// KEY1     =some_value
// LONGERKEY=some_other_value
//
// Note 1: this class re-parses the whole file before each Get/Set operation
//         consider using something else if Get/Set's are going to be frequent
// Note 2: Set operations can mess up character encoding if locale does not match
class Config
{
protected:
    typedef std::stringstream SS;
public:
    enum Mode { read_only, read_write };    
    
    bool SetFile(CSTR &filename_, Mode mode_); // returns true if success
                                               // overlapping key causes fail
                                               // keys may have trailing ' ' before '='

    SS&  Get(CSTR &key); // sets fail() == true if non-readable or key nonexistent
                         // empty val renders fail() == false, extraction will set fail() == true
                         // subsequent call to Get invalidates previously obtained SS& 

    bool Set(CSTR &key, CSTR &val); // returns true if success
                                    // key exists : replaces content after first '='
                                    //              all other content remains same
                                    // key missing: adds "key=val" after last line

    Config() : mode(read_only) {}
    Config(CSTR &filename, Mode mode = read_only) { SetFile(filename, mode); }
protected:
    STR filename;
    Mode mode;
    SS ss;
    std::vector<STR> lines;
    std::map<STR, std::size_t> map_key_idx;
};

}

#endif /* SIBYL_UTIL_CONFIG_H */