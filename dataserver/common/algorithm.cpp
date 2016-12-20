// algorithm.cpp
//
#include "dataserver/common/algorithm.h"
//#include <cctype> // std::tolower

namespace sdl { namespace algo { namespace {

static_assert('a' == 97, "");
static_assert('z' == 122, "");
static_assert('A' == 65, "");
static_assert('Z' == 90, "");

inline constexpr int char_tolower(const int x) { // ASCII char
	return ((x >= 'A') && (x <= 'Z')) ? (x + 'a' - 'A') : x;
}

} // namespace

bool iequal(const char * first1, const char * const last1, const char * first2) {
    SDL_ASSERT(first1 && last1 && first2);
    SDL_ASSERT(!(last1 < first1));
    for (; first1 != last1; ++first1, ++first2) {
        SDL_ASSERT(*first1);
        SDL_ASSERT(*first2);
		if (char_tolower(*first1) != char_tolower(*first2)) {
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
			{
				constexpr const char s1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
				constexpr const char s2[] = "abcdefghijklmnopqrstuvwxyz";
				SDL_ASSERT(iequal(s1, s1 + count_of(s1) - 1, s2));
				SDL_ASSERT(icasecmp(s1, s2));
				SDL_ASSERT(icasecmp_n(s1, s2, count_of(s2) - 1));
				static_assert(char_tolower('A') == 'a', "");
				static_assert(char_tolower('Z') == 'z', "");
				static_assert(char_tolower('G') == 'g', "");
				static_assert(char_tolower('1') == '1', "");
			}
        }
    };
    static unit_test s_test;
}
} // algo
} // sdl
#endif //#if SV_DEBUG

