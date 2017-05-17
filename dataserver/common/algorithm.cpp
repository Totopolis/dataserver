// algorithm.cpp
//
#include "dataserver/common/algorithm.h"
#include "dataserver/common/hash_combine.h"

namespace sdl { namespace algo { namespace {

static_assert('a' == 97, "");
static_assert('z' == 122, "");
static_assert('A' == 65, "");
static_assert('Z' == 90, "");

inline constexpr int char_tolower(const int x) { // ASCII char
    return ((x >= 'A') && (x <= 'Z')) ? (x + 'a' - 'A') : x;
}

} // namespace

bool iequal_range(const char * first1, const char * const last1, const char * first2) {
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
            bool unit_test_done = false;
            SDL_UTILITY_SCOPE_EXIT([&unit_test_done](){
                SDL_ASSERT(unit_test_done);
            });
            SDL_UTILITY_ASSERT_EXIT([&unit_test_done](){
                return unit_test_done;
            });
            SDL_DEBUG_TIMER_SEC(test_timer, "seconds = ");
            SDL_ASSERT(number_of_1(0x1) == 1);
            SDL_ASSERT(number_of_1(0x101) == 2);
            SDL_ASSERT(number_of_1(0x111) == 3);
            SDL_ASSERT(number_of_1<uint8>(0x80) == 1);
            SDL_ASSERT(number_of_1<uint8>(0x88) == 2);
            SDL_ASSERT(number_of_1<int16>(-1) == 16);
            SDL_ASSERT(number_of_1<int32>(-1) == 32);
            SDL_ASSERT(number_of_1<int64>(-1) == 64);
            {
                constexpr const char s1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
                constexpr const char s2[] = "abcdefghijklmnopqrstuvwxyz";
                SDL_ASSERT(iequal_range(s1, s1 + count_of(s1) - 1, s2));
                SDL_ASSERT(iequal(s1, s2));
                SDL_ASSERT(iequal_n(s1, s2, count_of(s2) - 1));
                static_assert(char_tolower('A') == 'a', "");
                static_assert(char_tolower('Z') == 'z', "");
                static_assert(char_tolower('G') == 'g', "");
                static_assert(char_tolower('1') == '1', "");
            }
            {
                constexpr const char s1[] = "/GeocodeOne";        
                constexpr const char s2[] = "/GeocodeOne?";        
                const char * const p1 = s1;
                const char * const p2 = s2;
                SDL_ASSERT(!iequal(s1, s2));
                SDL_ASSERT(!iequal_n(s1, s2));
                SDL_ASSERT(iequal_n(s2, s1));
                //
                SDL_ASSERT(!iequal(p1, p2));
                SDL_ASSERT(!iequal_n(p1, p2));
                SDL_ASSERT(iequal_n(p2, p1));
                //
                SDL_ASSERT(iequal("/geocodeONE", s1));
                SDL_ASSERT(iequal_n("/geocodeONE?1,2,3", s2, count_of(s2) - 1));
                SDL_ASSERT(iequal_n("/geocodeONE?1,2,3", s2));
                SDL_ASSERT(!iequal_n("/geocodeONE", s2, count_of(s2) - 1));
                SDL_ASSERT(!iequal_n("/geocodeONE", s2));
                SDL_ASSERT(!iequal("/geocodeONE?1,2,3", s2));
                //
                SDL_ASSERT(iequal(std::string("/geocodeONE"), s1));
                SDL_ASSERT(iequal_n(std::string("/geocodeONE?1,2,3"), s2, count_of(s2) - 1));
                SDL_ASSERT(iequal_n(std::string("/geocodeONE?1,2,3"), s2));
                SDL_ASSERT(!iequal_n(std::string("/geocodeONE"), s2, count_of(s2) - 1));
                SDL_ASSERT(!iequal_n(std::string("/geocodeONE"), s2));
                SDL_ASSERT(!iequal(std::string("/geocodeONE?1,2,3"), s2));
                //
                SDL_ASSERT(iequal(std::string("/geocodeONE"), p1));
                SDL_ASSERT(iequal_n(std::string("/geocodeONE?1,2,3"), p2));
                SDL_ASSERT(!iequal_n(std::string("/geocodeONE"), p2));
                SDL_ASSERT(!iequal(std::string("/geocodeONE?1,2,3"), p2));
                {
                    std::vector<int> test = { 2, 1, 2, 3, 2, 1 };
                    sort_erase_unique(test);
                    SDL_ASSERT(test.size() == 3);
                    SDL_ASSERT(is_sorted(test));
                }
            }
            {
                std::size_t seed = 0;
                hash_detail::hash_combine(seed, 0); SDL_ASSERT(seed);
                hash_detail::hash_combine(seed, 0); SDL_ASSERT(seed); 
                hash_detail::hash_combine(seed, 0); SDL_ASSERT(seed);
            }
            {
                SDL_ASSERT(to_upper("aBcD123") == "ABCD123");
                SDL_ASSERT(to_lower("aBcD123") == "abcd123");
            }
            unit_test_done = true;
        }
    };
    static unit_test s_test;
}
} // algo
} // sdl
#endif //#if SV_DEBUG

