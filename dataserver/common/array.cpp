// array.cpp
//
#include "common.h"
#include "array.h"

#if SDL_DEBUG
namespace sdl { namespace {
    class unit_test {
    public:
        unit_test()
        {
            if (1) {
                using T = vector_buf<size_t, 16>;
                T test;
                SDL_ASSERT(test.size() == 0);
                SDL_ASSERT(test.capacity() == T::BUF_SIZE);
                enum { N = T::BUF_SIZE * 2 };
                for (size_t i = 0; i < T::BUF_SIZE; ++i) {
                    test.push_back(N - i);
                }
                SDL_ASSERT(test.use_buf());
                SDL_ASSERT(test.size() == T::BUF_SIZE);
                SDL_ASSERT(test.capacity() == T::BUF_SIZE);
                test.sort();
                SDL_ASSERT(test[0] < test[test.size() - 1]);
                SDL_ASSERT(std::is_sorted(test.begin(), test.end()));
                for (size_t i = T::BUF_SIZE; i < N; ++i) {
                    test.push_back(N - i);
                }
                SDL_ASSERT(!test.use_buf());
                SDL_ASSERT(test.size() == N);
                SDL_ASSERT(test.capacity() >= N);
                test.sort();
                SDL_ASSERT(test[0] < test[test.size() - 1]);
                SDL_ASSERT(std::is_sorted(test.begin(), test.end()));
                test.fill_0();
            }
        }
    };
    static unit_test s_test;
}} // sdl
#endif //#if SV_DEBUG
