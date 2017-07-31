// interval_set.cpp
//
#include "dataserver/spatial/interval_set.h"
#include "dataserver/common/algorithm.h"

#if SDL_DEBUG
namespace sdl {
    namespace db {
        template<> struct interval_distance<std::string> {
            using pk0_type = std::string;
            int operator()(pk0_type const x, pk0_type const y) const {
                return atoi(y.c_str()) - atoi(x.c_str());
            }
        };
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    test_1();
                    test_2();
                    test_3();
                }
            private:
                static void test_1() {
                    using pk0_type = int64 ;
                    using test_interval_set = interval_set<pk0_type>;
                    test_interval_set test;
                    if (1) {
                        SDL_ASSERT(test.insert(4));
                        SDL_ASSERT(test.back() == 4);
                        SDL_ASSERT(test.insert(5));
                        SDL_ASSERT(test.back() == 5);
                        SDL_ASSERT(test.back2() == test_interval_set::pair_value(4,5));
                        SDL_ASSERT(test.insert(10));
                        SDL_ASSERT(!test.insert(10));
                        {
                            SDL_ASSERT(test.erase_back2() == 1);
                            SDL_ASSERT(test.size() == 2);
                            SDL_ASSERT(test.erase_back2() == 2);
                            SDL_ASSERT(test.empty());
                            SDL_ASSERT(test.insert(4));
                            SDL_ASSERT(test.insert(5));
                            SDL_ASSERT(test.insert(10));
                        }
                        test.insert(2);
                        test.insert(3);
                        for (pk0_type i = 0; i < 5; ++i) {
                            test.insert(i + 1);
                        }
                        std::vector<pk0_type> check;
                        test.for_each([&check](pk0_type v){
                            check.push_back(v);
                            return bc::continue_;
                        });
                        SDL_ASSERT(check.size() == test.size());
                        SDL_ASSERT(algo::is_sorted(check));
                        auto it = check.begin();
                        for (pk0_type v1 : test) {
                            pk0_type v2 = *it++;
                            SDL_ASSERT(v1 == v2);
                        }
                        SDL_ASSERT((size_t)std::distance(test.begin(), test.end()) == test.size());
                    }
                    if (1) {
                        test.insert(8);
                        test.insert(7);
                        test.insert(12);
                        test.insert(10);
                        test.insert(11);
                        test.insert(9);
                        test.insert(13);
                        test.insert(16);
                        test.insert(17); 
                    }
                    if (1) {
                        test.insert(2); 
                        test.insert(1); 
                    }
                    if (1) {
                        test.insert(0);
                    }
                    if (1) {
                        pk0_type old = std::numeric_limits<pk0_type>::min();
                        size_t count_cell = 0;
                        break_or_continue res = test.for_each([&count_cell, &old](pk0_type cell){
                            SDL_ASSERT(!(cell < old));
                            old = cell;
                            ++count_cell;
                            return bc::continue_;
                        });
                        SDL_ASSERT(count_cell == test.size());
                        SDL_ASSERT(res == bc::continue_);
                        SDL_ASSERT(!test.empty());
                        test_interval_set test2;
                        test.swap(test2);
                        SDL_ASSERT(!test2.empty());
                    }
                    test.clear();
                }
                static void test_2() {
                    using pk0_type = std::string;
                    using test_interval_set = interval_set<pk0_type>;
                    test_interval_set test;
                    test.insert("1");
                    test.insert("3");
                    test.insert("5");
                    SDL_ASSERT(test.insert("2"));
                    SDL_ASSERT(!test.insert("2"));
                    SDL_ASSERT(test.size() == 4);
                }
                template<typename value_type>
                static void test_set(value_type const v1, value_type const v2) {
                    const size_t set_count = v2 - v1 + 1;
                    using set_type = interval_set<value_type>;
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
                    check = v1;
                    for (value_type v : test) {
                        SDL_ASSERT(check == v);
                        ++check;
                    }
                    SDL_ASSERT(test.contains() <= test.size());
                    SDL_ASSERT((size_t)std::distance(test.begin(), test.end()) == set_count);
                    SDL_ASSERT(algo::is_sorted(test));
                    SDL_ASSERT(!test.empty());
                    set_type test2;
                    test2.swap(test);
                    SDL_ASSERT(test2.size() == set_count);
                    SDL_ASSERT(test.empty());
                    set_type test3(std::move(test2));
                    SDL_ASSERT(test3.size() == set_count);
                    SDL_ASSERT(test2.empty());
                }
                static void test_3()
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
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SDL_DEBUG
