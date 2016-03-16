// maketable.cpp
//
#include "common/common.h"
#include "maketable.h"

namespace sdl { namespace db {  namespace make {

/*tab.scan([](auto row) {});
tab.select({ if row return true; });
tab.find_pk(int[]);
tab.find_less(int, use_indexes);
tab.find_less(300).select( {bool} );
tab.find_less(300).top(10); // last(10)
tab.top(5).union( tab.last(5) );
tab.find( {bool} ) 
*/

#if SDL_DEBUG
namespace {
    template <class type_list> struct processor;

    template <> struct processor<NullType> {
        static void test(){}
    };
    template <class T, class U> // T = meta::col_type
    struct processor< Typelist<T, U> > {
        static void test(){
            T::test();
            processor<U>::test();
        }
    };
    void test_sample_table(sample::dbo_table * const table) {
        if (!table) return;
        using T = sample::dbo_table;
        static_assert(T::col_size == 2, "");
        static_assert(T::col_fixed, "");
        static_assert(sizeof(T::record) == 8, "");
        static_assert(std::is_assignable<T::record, T::record>::value, "");
        T & tab = *table;
        for (auto p : tab) {
            if (p.Id()) {}
        }
        tab.query.scan_if([](T::record){
            return true;
        });
        auto found = tab.query.find([](T::record p){
            return p.Id() > 0;
        });
        auto range = tab.query.select([](T::record p){
            return p.Id() > 0;
        });
    }
    class unit_test {
    public:
        unit_test() {
            struct col {
                using t_int                 = meta::col<0, scalartype::t_int, 4, meta::key_true>;
                using t_bigint              = meta::col<0, scalartype::t_bigint, 8>;
                using t_smallint            = meta::col<0, scalartype::t_smallint, 2>;
                using t_float               = meta::col<0, scalartype::t_float, 8>;
                using t_real                = meta::col<0, scalartype::t_real, 4>;
                using t_smalldatetime       = meta::col<0, scalartype::t_smalldatetime, 4>;
                using t_uniqueidentifier    = meta::col<0, scalartype::t_uniqueidentifier, 16>;
                using t_char                = meta::col<0, scalartype::t_char, 255>;
                using t_nchar               = meta::col<0, scalartype::t_nchar, 255>;
                using t_varchar             = meta::col<0, scalartype::t_varchar, 255>;
                using t_geometry            = meta::col<0, scalartype::t_geometry, -1>;
            };
            typedef TL::Seq<
                col::t_int
                ,col::t_bigint
                ,col::t_smallint
                ,col::t_float
                ,col::t_real
                ,col::t_smalldatetime
                ,col::t_uniqueidentifier
                ,col::t_char
                ,col::t_nchar
                ,col::t_varchar
                ,col::t_geometry
            >::Type type_list;
            processor<type_list>::test();
            test_sample_table(nullptr);
        }
    };
    static unit_test s_test;
}
#endif //#if SV_DEBUG

} // make
} // db
} // sdl
