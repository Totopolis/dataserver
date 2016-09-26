// vector_set.cpp
//
#include "common.h"
#include "vector_set.h"

#if SDL_DEBUG
namespace sdl { namespace {
    class unit_test {
    public:
        unit_test() {
            using T1 = vector_set<int, std::less<int>, equal_to<int, std::less<int>>>;
            using T2 = vector_set<int>;
            T1 v1;
            T2 v2;
            for (int i = 0; i < 10; ++i) {
                v1.insert(i);
                v1.insert(i);
                v1.insert((i + 5) % 10);
                v2.insert(i);
                v2.insert(i);
                v2.insert((i + 5) % 10);
            }
            SDL_ASSERT(v1.size() == 10);
            SDL_ASSERT(v2.size() == 10);
            for (int i : v1) {}
            for (int i : v2) {}
            v1.erase(v1.begin(), v1.end());
            SDL_ASSERT(v1.empty());
            SDL_ASSERT(*v2.find(5) == 5);
        }
    };
    static unit_test s_test;
}} // sdl
#endif //#if SV_DEBUG