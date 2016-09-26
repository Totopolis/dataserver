// interval_set.cpp
//
#include "common/common.h"
#include "interval_set.h"

#if SDL_DEBUG
namespace sdl {
    namespace db {
#if (SDL_DEBUG > 1) 
        template<> struct next_key<std::string> {
            std::string operator()(std::string const & x) const {
                char buf[64]{};
                const int temp = atoi(x.c_str()) + 1;
                return itoa(temp, buf, 10);
            }
        };
#endif
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
                    /*if (0) {
                        const size_t max_try = 3;
                        const size_t max_i[max_try] = {5000, 100000, 700000};
                        for (size_t j = 0; j < max_try; ++j) {
                            SDL_TRACE("\ninterval_set random test (", max_i[j], ")");
                            test_interval_set test2;
                            std::set<pk0_type> unique;
                            size_t count = 0;
                            for (size_t i = 0; i < max_i[j]; ++i) {
                                const double r1 = double(rand()) / RAND_MAX;
                                const double r2 = double(rand()) / RAND_MAX;
                                const uint32 _32 = static_cast<uint32>(uint32(-1) * r1 * r2);
                                if (unique.insert(_32).second) {
                                    ++count;
                                }
                                test2.insert(_32);
                                SDL_ASSERT(test2.find(_32));
                            }
                            SDL_ASSERT(count == unique.size());
                            size_t const test_count = test2.size();
                            SDL_ASSERT(count == test_count);
                            test2.for_each([&unique, &count](pk0_type const x){
                                size_t const n = unique.erase(x);
                                SDL_ASSERT(n == 1);
                                --count;
                                return bc::continue_;
                            });
                            SDL_ASSERT(count == 0);
                            SDL_ASSERT(unique.empty());
                        }
                    }*/
                }
                void test_2() {
#if (SDL_DEBUG > 1) 
                    using pk0_type = std::string;
                    using test_interval_set = interval_set<pk0_type>;
                    test_interval_set test;
                    test.insert("1");
                    test.insert("3");
                    test.insert("5");
                    test.insert("2");
#endif
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
