// union_find.cpp
//
#include "dataserver/numeric/union_find.h"

#if SDL_DEBUG
namespace sdl { namespace {
class unit_test {
public:
    unit_test() {
        if (1) {
            static const int data[][2] = {
                {4, 3},
                {3, 8},
                {6, 5},
                {9, 4},
                {2, 1},
                {8, 9},
                {5, 0},
                {7, 2},
                {6, 1},
                {1, 0},
                {6, 7},
            };
            {
                union_find_int test(10);
                for (int j = 0; j < 2; ++j) {
                    SDL_ASSERT(test.count() == 10);
                    for (auto i : data) {
                        test.unite(i[0], i[1]);
                    }
                    SDL_ASSERT(test.count() == 2);
                    SDL_ASSERT(test.added() == 10);
                    test.clear();
                }
            }
            {
                union_find_t<size_t> test(10);
                for (auto i : data) {
                    test.unite(i[0], i[1]);
                }
                SDL_ASSERT(test.count() == 2);
            }
            {
                union_find_t<std::uint8_t> test(10);
                for (auto i : data) {
                    test.unite(i[0], i[1]);
                }
                SDL_ASSERT(test.count() == 2);
            }
        }
    }
};
static unit_test s_test;
}} // sdl
#endif //#if SDL_DEBUG
