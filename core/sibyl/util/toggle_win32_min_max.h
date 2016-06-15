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