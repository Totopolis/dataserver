// interval_set.cpp
//
#include "common/common.h"
#include "interval_set.h"

#if SDL_DEBUG
namespace sdl {
    namespace db {
        template<> struct interval_distance<std::string> {
            using pk0_type = std::string;
            size_t operator()(pk0_type const x, pk0_type const y) const {
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
                }
            private:
                void test_1() {
                    using pk0_type = int64 ;
                    using test_interval_set = interval_set<pk0_type>;
                    test_interval_set test;
                    if (1) {
                        test.insert(4);
                        test.insert(5);
                        test.insert(10);
                        test.insert(2);
                        test.insert(3);
                        for (pk0_type i = 0; i < 5; ++i) {
                            test.insert(i + 1);
                        }
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
                void test_2() {
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
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
