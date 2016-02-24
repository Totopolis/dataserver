// index_tree.cpp
//
#include "common/common.h"
#include "index_tree.h"
#include "database.h"

namespace sdl { namespace db {

} // db
} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    SDL_TRACE_FILE;
                    using K1 = index_key<scalartype::t_int>::type;
                    using K2 = index_key<scalartype::t_bigint>::type;
                    using T1 = index_tree<K1>;
                    using T2 = index_tree<K2>;
                    using T11 = index_tree_t<scalartype::t_int>;
                    using T22 = index_tree_t<scalartype::t_bigint>;
                    A_STATIC_ASSERT_TYPE(T1, T11);
                    A_STATIC_ASSERT_TYPE(T2, T22);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG