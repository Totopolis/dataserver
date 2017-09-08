// indexx.cpp
//
#include "dataserver/common/indexx.h"
#include "dataserver/common/algorithm.h"

#if 0 //SDL_DEBUG
namespace sdl { namespace {
class unit_test {
public:
    unit_test() {
        if (1) {
            using indexx = small::indexx_uint8;
            using T = indexx::value_type;
            enum { N = indexx::capacity() };
            enum { M = 3 };
            indexx test;            
            for (size_t i = 0; i < N * M; ++i) {
                SDL_ASSERT(test.insert() < N);
            }
            SDL_ASSERT(test.size() == N);
            SDL_ASSERT(algo::is_sorted_desc(test));
            SDL_ASSERT(algo::is_unique(test));
            for (size_t i = 0; i < M; ++i) {
                SDL_ASSERT(test.select(0) < N);
                SDL_ASSERT(test.select(N - 1) < N);
                SDL_ASSERT(test.select(N / 2) < N);
                enum { select = 0 };
                SDL_ASSERT(test.find([](T const i) { 
                    return i == select; 
                }));
                if (test.find_and_select([](T const i) { 
                    return i == select; 
                }).second) {
                    SDL_ASSERT(*test.begin() == select);
                }
            }
            std::vector<T> v(test.begin(), test.end());
            algo::sort(v);
            SDL_ASSERT(algo::is_unique(v));
        }
    }
};
static unit_test s_test;
}} // sdl
#endif //#if SDL_DEBUG
