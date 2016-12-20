// algorithm.cpp
//
#include "dataserver/common/algorithm.h"
#include <cctype>

namespace sdl { namespace algo {

bool iequal(const char * first1, const char * const last1, const char * first2) {
    SDL_ASSERT(first1 && last1 && first2);
    SDL_ASSERT(!(last1 < first1));
    for (; first1 != last1; ++first1, ++first2) {
        SDL_ASSERT(*first1);
        SDL_ASSERT(*first2);
        if (std::tolower(*first1) != std::tolower(*first2)) {
            return false;
        }
    }
    return true;
}

} // algo
} // sdl

#if SDL_DEBUG
namespace sdl { namespace algo { namespace {
    class unit_test {
    public:
        unit_test()
        {
            SDL_ASSERT(number_of_1(0x1) == 1);
            SDL_ASSERT(number_of_1(0x101) == 2);
            SDL_ASSERT(number_of_1(0x111) == 3);
            SDL_ASSERT(number_of_1<uint8>(0x80) == 1);
            SDL_ASSERT(number_of_1<uint8>(0x88) == 2);
            SDL_ASSERT(number_of_1<int16>(-1) == 16);
            SDL_ASSERT(number_of_1<int32>(-1) == 32);
            SDL_ASSERT(number_of_1<int64>(-1) == 64);
            {
                constexpr const char s1[] = "/GeocodeOne";        
                constexpr const char s2[] = "/GeocodeOne?";        
                SDL_ASSERT(icasecmp("/geocodeONE", s1));
                SDL_ASSERT(icasecmp_n("/geocodeONE?1,2,3", s2, count_of(s2) - 1));
                SDL_ASSERT(!icasecmp_n("/geocodeONE", s2, count_of(s2) - 1));
                SDL_ASSERT(!icasecmp("/geocodeONE?1,2,3", s2));
            }
        }
    };
    static unit_test s_test;
}
} // algo
} // sdl
#endif //#if SV_DEBUG

