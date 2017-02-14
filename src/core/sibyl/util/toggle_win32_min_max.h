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

// Note: include guard intentionally omitted

// Wrap a block of code to be protected from min & max macros defined in minwindef.h
// 
// #include "toggle_win32_min_max_h"
// /*
//     some code that uses std::min or std::max
// */
// #include "toggle_win32_min_max_h"
// 

#ifdef _WIN32
    #ifdef min
        #pragma push_macro("min")
        #undef min
    #else
        #define min
        #pragma pop_macro("min")
    #endif /* min */

    #ifdef max
        #pragma push_macro("max")
        #undef max
    #else
        #define max
        #pragma pop_macro("max")
    #endif /* max */
#endif /* _WIN32 */

// Note: include guard intentionally omitted