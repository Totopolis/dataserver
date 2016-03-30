// maketable.cpp
//
#include "common/common.h"
#include "maketable.h"

/*tab.scan([](auto row) {});
tab.select({ if row return true; });
tab.find_pk(int[]);
tab.find_less(int, use_indexes);
tab.find_less(300).select( {bool} );
tab.find_less(300).top(10); // last(10)
tab.top(5).union( tab.last(5) );
tab.find( {bool} ) 
*/

#if 0 // reserved
namespace sdl { namespace db { namespace make {
template<class META>
class base_cluster: public META {
    using TYPE_LIST = typename META::type_list;
    base_cluster() = delete;
public:
    enum { index_size = TL::Length<TYPE_LIST>::value };        
    template<size_t i> using index_col = typename TL::TypeAt<TYPE_LIST, i>::Result;
private:
    template<size_t i> static 
    const void * get_address(const void * const begin) {
        static_assert(i < index_size, "");
        return reinterpret_cast<const char *>(begin) + index_col<i>::offset;
    }
    template<size_t i> static 
    void * set_address(void * const begin) {
        static_assert(i < index_size, "");
        return reinterpret_cast<char *>(begin) + index_col<i>::offset;
    }
protected:
    template<size_t i> static 
    auto get_col(const void * const begin) -> meta::index_type<TYPE_LIST, i> const & {
        using T = meta::index_type<TYPE_LIST, i>;
        return * reinterpret_cast<T const *>(get_address<i>(begin));
    }
    template<size_t i> static 
    auto set_col(void * const begin) -> meta::index_type<TYPE_LIST, i> & {
        using T = meta::index_type<TYPE_LIST, i>;
        return * reinterpret_cast<T *>(set_address<i>(begin));
    }
};
} // make
} // db
} // sdl
#endif

#if SDL_DEBUG
namespace sdl { namespace db {  namespace make { namespace sample { namespace {

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
        using T = sample::dbo_table;
        static_assert(T::col_size == 3, "");
        static_assert(T::col_fixed, "");
        static_assert(sizeof(T::record) == 8, "");
        using clustered = T::clustered;
        using query_type = T::query_type;
        using key_type = clustered::key_type;
        static_assert(clustered::index_size == 2, "");
        static_assert(sizeof(key_type) ==
            sizeof(clustered::T0::type) +
            sizeof(clustered::T1::type),
            "");
        static_assert(clustered::T0::offset == 0, "");
        static_assert(clustered::T1::offset == 4, "");
        A_STATIC_ASSERT_IS_POD(key_type);
        if (table) {
            T & tab = *table;
            for (auto p : tab) {
                if (p.Id()) {}
            }
            tab->scan_if([](T::record){
                return true;
            });
            if (auto found = tab->find([](T::record p){
                return p.Id() > 0;
            })) {
                SDL_ASSERT(found.Id() > 0);
            }
            std::vector<T::record> range;
            tab->scan_if([&range](T::record p){
                if (p.Id() > 0) {
                    range.push_back(p);
                    return true;
                }
                return false;
            });
            key_type test{};
            auto _0 = test.get<0>();
            auto _1 = test.get<1>();
            static_assert(std::is_same<int const &, decltype(test.get<0>())>::value, "");
            static_assert(std::is_same<uint64 const &, decltype(test.get<1>())>::value, "");
            test.set<0>() = _0;
            test.set<1>() = _1;
            static_assert(std::is_same<int &, decltype(test.set<0>())>::value, "");
            static_assert(std::is_same<uint64 &, decltype(test.set<1>())>::value, "");
            const auto key = tab->read_key(tab->find([](T::record){ return true; }));
            if (auto p = tab->find_with_index(key)) {
                A_STATIC_CHECK_TYPE(T::record, p);
            }
            tab->select({ tab->make_key(1, 2), tab->make_key(2, 1) });
            tab->select(tab->make_key(1, 2));
            const std::vector<key_type> keys({
                tab->make_key(1, 2), 
                tab->make_key(2, 1) });
            tab->select(keys);
        }
        if (1) {
            using S = query_type;
            const key_type x = S::make_key(1, 2);
            const key_type y = S::make_key(2, 1);
            SDL_ASSERT(x._0 == 1);
            SDL_ASSERT(x._1 == 2);
            SDL_ASSERT(x < y);
            SDL_ASSERT(x != y);
            SDL_ASSERT(x == x);
            auto in = { S::make_key(0, 1), S::make_key(1, 1) };
            for (auto const & k : in) {
                A_STATIC_CHECK_TYPE(key_type const &, k);
                SDL_ASSERT(!meta::key_to_string<clustered::type_list>::to_str(k).empty());
            }
        }
    }
    class unit_test {
    public:
        unit_test() {
            struct col {
                using t_int                 = meta::col<0, 0, scalartype::t_int, 4, meta::key_true>;
                using t_bigint              = meta::col<0, 0, scalartype::t_bigint, 8>;
                using t_smallint            = meta::col<0, 0, scalartype::t_smallint, 2>;
                using t_float               = meta::col<0, 0, scalartype::t_float, 8>;
                using t_real                = meta::col<0, 0, scalartype::t_real, 4>;
                using t_smalldatetime       = meta::col<0, 0, scalartype::t_smalldatetime, 4>;
                using t_uniqueidentifier    = meta::col<0, 0, scalartype::t_uniqueidentifier, 16>;
                using t_char                = meta::col<0, 0, scalartype::t_char, 255>;
                using t_nchar               = meta::col<0, 0, scalartype::t_nchar, 20>;
                using t_varchar             = meta::col<0, 0, scalartype::t_varchar, 255>;
                using t_geography            = meta::col<0, 0, scalartype::t_geography, -1>;
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
                ,col::t_geography
            >::Type type_list;
            processor<type_list>::test();
            test_sample_table(nullptr);
            if (0) {
                SDL_TRACE(typeid(sample::dbo_META::col::Id).name());
                SDL_TRACE(typeid(sample::dbo_META::col::Col1).name());
            }
            if (0) {
                make::index_tree<dbo_META::clustered::key_type> test(nullptr, nullptr);
            }
        }
    };
    static unit_test s_test;

} // namespace
} // sample
} // make
} // db
} // sdl
#endif //#if SV_DEBUG

