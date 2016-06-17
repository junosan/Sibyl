/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include "Config.h"

#include <iostream>
#include <fstream>
#include <algorithm>

namespace sibyl
{

#include "toggle_win32_min_max.h"

bool Config::SetFile(CSTR &filename_, Config::Mode mode_)
{
    filename = filename_;
    mode     = mode_;
    lines.clear();
    map_key_idx.clear();
    bool success = true;
    std::ifstream ifs(filename);
    if (ifs.is_open() == true)
    {
        for (STR line; std::getline(ifs, line);)
        {
            if (line.back() == '\r') line.pop_back();
            auto posEq = line.find_first_of('=');
            if (posEq != std::string::npos)
            {
                auto keylen = std::min(posEq, line.find_first_of(' '));
                STR key(line, 0, keylen);
                auto it_success = map_key_idx.insert(std::make_pair(key, lines.size()));
                if (it_success.second == false)
                {
                    std::cerr << "Config::SetFile: Overlapping key " << key << " found" << std::endl;
                    success = false;
                    break;
                }
            }
            lines.push_back(line);            
        }
    }
    else
    {
        if (mode == read_write)
        {
            std::ofstream ofs(filename, std::ios::app);
            if (ofs.is_open() == false)
            {
                std::cerr << "Config::SetFile: Cannot write " << filename << std::endl;
                success = false;
            }
            // else: writing a new file (success = true)
        }
        else
        {
            std::cerr << "Config::SetFile: Cannot read " << filename << std::endl;
            success = false;
        }
    }
    if (success == false) filename.clear();
    return success;
}

#include "toggle_win32_min_max.h"

Config::SS& Config::Get(CSTR &key)
{
    bool success = false;
    if (filename.empty() == true)
        std::cerr << "Config::Get: Valid SetFile call must precede this" << std::endl;
    else if (SetFile(filename, mode) == true)
    {
        auto it = map_key_idx.find(key);
        if (it != std::end(map_key_idx))
        {
            STR line = lines.at(it->second);
            auto posEq = line.find_first_of('=');
            STR val = line.substr(posEq + 1);
            ss.clear();
            ss.str(val);
            success = true;        
        }
    }
    if (success == false) ss.setstate(std::ios_base::failbit);
    return ss;
}

bool Config::Set(CSTR &key, CSTR &val)
{
    bool success = false;
    if (mode == read_only)
        std::cerr << "Config::Set: Cannot set in read_only mode" << std::endl;
    else if (filename.empty() == true)
        std::cerr << "Config::Set: Valid SetFile call must precede this" << std::endl;
    else if (SetFile(filename, mode) == true)
    {
        std::size_t n = lines.size();
        STR line;        // line to be written
        std::size_t idx; // idx of line to be written
        auto it = map_key_idx.find(key);
        if (it != std::end(map_key_idx))
        {
            STR prev = lines.at(it->second);
            auto posEq = prev.find_first_of('=');
            prev.resize(posEq + 1);
            line = prev + val;
            idx = it->second;
        }
        else
        {
            line = key + '=' + val;
            idx = n;
        }
        std::ofstream ofs(filename, std::ios::trunc);
        for (std::size_t i = 0; i < n + 1; i++)
        {
            if   (i == idx) ofs << line        << '\n';
            else if (i < n) ofs << lines.at(i) << '\n';
        }
        success = true;
    }
    return success;
}

}
