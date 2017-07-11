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
        }
    };
    static unit_test s_test;
}
#endif // SDL_DEBUG

} // sdl