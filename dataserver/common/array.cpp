// array.cpp
//
#include "dataserver/common/array.h"

#if SDL_DEBUG
namespace sdl { namespace {
    class unit_test {
    public:
        unit_test()
        {
            if (1) {
                unique_vec<size_t> v1;
                SDL_ASSERT(v1.empty());
                SDL_ASSERT(!v1);
                SDL_ASSERT(v1->empty());
                SDL_ASSERT(!v1);
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
                std::sort(test.begin(), test.end());
                test.push_sorted(100);
                test.push_sorted(100);
                test.push_sorted(99);
                test.push_sorted(0);
                test.push_sorted(97);
                test.push_sorted(98);
                test.push_sorted(98);
                test.push_sorted(101);
                SDL_ASSERT(test[0] < test[test.size() - 1]);
                SDL_ASSERT(std::is_sorted(test.begin(), test.end()));
                for (size_t i = T::BUF_SIZE; i < N; ++i) {
                    test.push_back(N - i);
                }
                SDL_ASSERT(!test.use_buf());
                std::sort(test.begin(), test.end());
                SDL_ASSERT(test[0] < test[test.size() - 1]);
                SDL_ASSERT(std::is_sorted(test.begin(), test.end()));
                test.fill_0();
            }
        }
    };
    static unit_test s_test;
}
} // sdl
#endif //#if SV_DEBUG


#if 0
template <class ForwardIterator>
  void rotate (ForwardIterator first, ForwardIterator middle,
               ForwardIterator last)
{
  ForwardIterator next = middle;
  while (first!=next)
  {
    swap (*first++,*next++);
    if (next==last) next=middle;
    else if (first==middle) middle=next;
  }
}
#endif