// spinlock.cpp
#include "dataserver/common/spinlock.h"

namespace sdl {

#if SDL_DEBUG
namespace {
    struct unit_test {
        unit_test() {
            if (0) {
                atomic_flag_init f;
                std::vector<std::thread> v;
                for (int n = 0; n < 10; ++n) {
                    v.emplace_back([&f, n](){
                        spin_lock lock(f.value);
                        for (int cnt = 0; cnt < 100; ++cnt) {
                            std::cout << cnt << ": Output from thread " << n << '\n';
                        }
                    });
                }
                for (auto& t : v) {
                    t.join();
                }
            }
#if 1 // reserved
            if (1) {
                enum { N = 10 };
                std::atomic<uint32> counter;
                counter = 1;
                {
                    test::readlock reader(counter);
                    SDL_ASSERT(reader.try_lock());
                    SDL_ASSERT(reader.commit());
                    int x = 0;
                    reader.work<int>(x, [](int & x){ 
                        x = 255; 
                    });
                    SDL_ASSERT(255 == x);
                }
                for (size_t i = 0; i < N; ++i) {
                    SDL_ASSERT((i+1) == counter);
                    test::writelock writer(counter);
                    test::readlock reader(counter);
                    SDL_ASSERT(!reader.try_lock());
                    SDL_ASSERT(!reader.commit());
                }
                SDL_ASSERT((N+1) == counter);
            }
#endif
        }
    };
    static unit_test s_test;
}
#endif // SDL_DEBUG

} // sdl