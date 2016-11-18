// sparse_set.cpp
//
#include "dataserver/spatial/sparse_set.h"
#include "dataserver/common/algorithm.h"

#if SDL_DEBUG
namespace sdl { namespace db { namespace {
    class unit_test {
    public:
        unit_test()
        {
            test_set<uint64>(0, 10);
            test_set<uint64>(0, 63);
            test_set<uint64>(64, 127);
            test_set<int>(-1, 1);
            test_set<int>(-64, 64);
            test_set<int>(-65, 65);
            test_set<int64>(-10, 10);
            test_set<int64>(0, 10);
            test_set<unsigned int>(0, 10);
            test_set<int>(0, 10);
            test_set<int>(-10, 10);
            {
                test_set<uint16>(0, 10);
                test_set<uint16>(0, 63);
                test_set<uint16>(64, 127);
                test_set<int16>(-1, 1);
                test_set<int16>(-64, 64);
                test_set<int16>(-65, 65);
            }
        }
    private:
        template<typename value_type>
        static void test_set(value_type const v1, value_type const v2) {
            const size_t set_count = v2 - v1 + 1;
            using set_type = sparse_set<value_type>;
            set_type test;
            SDL_ASSERT(test.empty());
            for (value_type i = v1; i < v2; ++i) {
                SDL_ASSERT(test.insert(i));
            }
            SDL_ASSERT(test.size() == set_count - 1);
            SDL_ASSERT(test.insert(v2));
            SDL_ASSERT(test.size() == set_count);
            value_type check = v1;
            test.for_each([&check](value_type v){
                SDL_ASSERT(check == v);
                ++check;
                return true;
            });
            SDL_ASSERT(test.contains() <= test.size());
            SDL_ASSERT(std::distance(test.begin(), test.end()) == set_count);
            auto const vec = test.copy_to_vector();
            SDL_ASSERT(algo::is_sorted(vec));
            SDL_ASSERT(algo::is_sorted(test));
            {
                SDL_ASSERT(vec.size() == test.size());
                auto it = vec.begin();
                for (auto value : test) {
                    SDL_ASSERT(value == *it++);
                }
            }
            SDL_ASSERT(!test.empty());
            set_type test2;
            test2.swap(test);
            SDL_ASSERT(test2.size() == set_count);
            SDL_ASSERT(test.empty());
            set_type test3(std::move(test2));
            SDL_ASSERT(test3.size() == set_count);
            SDL_ASSERT(test2.empty());
        }
    };
    static unit_test s_test;
}
} // db
} // sdl
#endif //#if SV_DEBUG