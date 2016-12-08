// maketable.cpp
//
#include "dataserver/maketable/maketable.h"

#if SDL_DEBUG
namespace sdl { namespace db { namespace make { namespace sample {
struct dbo_META {
    struct col {
        struct Id : meta::col<0, 0, scalartype::t_int, 4, meta::key<true, 0, sortorder::ASC>> { static constexpr const char * name() { return "Id"; } };
        struct Id2 : meta::col<1, 4, scalartype::t_bigint, 8, meta::key<true, 1, sortorder::DESC>> { static constexpr const char * name() { return "Id2"; } };
        struct Col1 : meta::col<2, 12, scalartype::t_char, 255> { static constexpr const char * name() { return "Col1"; } };
    };
    typedef TL::Seq<
        col::Id
        ,col::Id2
        ,col::Col1
    >::Type type_list;
    struct clustered_META {
        using T0 = meta::index_col<col::Id>;
        using T1 = meta::index_col<col::Id2, T0::offset + sizeof(T0::type)>;
        typedef TL::Seq<T0, T1>::Type type_list;
    };
    struct clustered final : make_clustered<clustered_META> {
#pragma pack(push, 1)
        struct key_type {
            T0::type _0;
            T1::type _1;
            void get(Int2Type<0>) const && = delete;
            void get(Int2Type<1>) const && = delete;
            void set(Int2Type<0>) && = delete;
            void set(Int2Type<1>) && = delete;
            T0::type const & get(Int2Type<0>) const & { return _0; }
            T1::type const & get(Int2Type<1>) const & { return _1; }
            T0::type & set(Int2Type<0>) & { return _0; }
            T1::type & set(Int2Type<1>) & { return _1; }
            template<size_t i> void get() && = delete;
            template<size_t i> void set() && = delete;
            template<size_t i> auto get() & -> decltype(get(Int2Type<i>())) { return get(Int2Type<i>()); }
            template<size_t i> auto set() & -> decltype(set(Int2Type<i>())) { return set(Int2Type<i>()); }
            using this_clustered = clustered;
        };
#pragma pack(pop)
        static const char * name() { return ""; }
        static bool is_less(key_type const & x, key_type const & y) {
            if (meta::is_less<T0>::less(x._0, y._0)) return true;
            if (meta::is_less<T0>::less(y._0, x._0)) return false;
            if (meta::is_less<T1>::less(x._1, y._1)) return true;
            return false; // keys are equal
        }
        static bool less_first(decltype(key_type()._0) const & x, decltype(key_type()._0) const & y) {
            if (meta::is_less<T0>::less(x, y)) return true;
            return false;
        }
        static constexpr pageType::type root_page_type = pageType::type::index;
    };
    static constexpr const char * name() { return ""; }
    static constexpr int32 id = 0;
};

class dbo_table final : public dbo_META, public make_base_table<dbo_META> {
    using base_table = make_base_table<dbo_META>;
    using this_table = dbo_table;
public:
    class record final : public base_record<this_table> {
        using base = base_record<this_table>;
        using access = base_access<this_table, record>;
        using query = make_query<this_table, record>;
        friend access;
        friend query;
        friend this_table;
        record(this_table const * p, row_head const * h) noexcept : base(p, h) {}
    public:
        record() = default;
        auto Id() const -> col::Id::ret_type { return val<col::Id>(); }
        auto Col1() const -> col::Col1::ret_type { return val<col::Col1>(); }
    };
    static constexpr size_t static_record_count = 0;
public:
    using iterator = record::access::iterator;
    using query_type = record::query;
    explicit dbo_table(database const * p, shared_usertable const & s)
        : base_table(p, s), _record(this), query(this, p)
    {}
    iterator begin() const { return _record.begin(); }
    iterator end() const { return _record.end(); }
    query_type * operator ->() { return &query; }
private:
    const record::access _record;
public:
    query_type query;
};

template <class type_list> struct test_processor;
template <> struct test_processor<NullType> {
    static void test(){}
};
template <class T, class U> // T = meta::col_type
struct test_processor< Typelist<T, U> > {
    static void test(){
        T::test();
        test_processor<U>::test();
    }
};
void test_sample_table(sample::dbo_table * const table) {
    using T = sample::dbo_table;
    static_assert(T::col_size == 3, "");
    static_assert(T::col_fixed, "");
    static_assert(sizeof(T::record) == sizeof(void *), "");
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
        tab.query.scan_if([](T::record){
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
        static_assert(std::is_same<int32 const &, decltype(test.get<0>())>::value, "");
        static_assert(std::is_same<int64 const &, decltype(test.get<1>())>::value, "");
        test.set<0>() = _0;
        test.set<1>() = _1;
        static_assert(std::is_same<int32 &, decltype(test.set<0>())>::value, "");
        static_assert(std::is_same<int64 &, decltype(test.set<1>())>::value, "");
        const auto key = tab->read_key(tab->find([](T::record){ return true; }));
        if (auto p = tab->find_with_index(key)) {
            A_STATIC_CHECK_TYPE(T::record, p);
        }
        if (1) {
            using namespace where_;
            tab->SELECT | WHERE<T::col::Id>{1} | LESS<T::col::Id2>{1} | GREATER<T::col::Id2>{2};
            tab->SELECT 
                | IN<T::col::Id>{1,2,3}
                && ORDER_BY<T::col::Id>{}
                && NOT<T::col::Id2>{1}
                && ORDER_BY<T::col::Col1>{}
                ;
            //FIXME: failed build on Ubuntu ?
            //auto r1 = (tab->SELECT | BETWEEN<T::col::Id>{1,2} && ORDER_BY<T::col::Id>{}).VALUES();
        }
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
        test_processor<type_list>::test();
        test_sample_table(nullptr);
        if (0) {
            SDL_TRACE(typeid(sample::dbo_META::col::Id).name());
            SDL_TRACE(typeid(sample::dbo_META::col::Col1).name());
        }
        if (0) {
            make::index_tree<dbo_META::clustered::key_type> test(nullptr, nullptr);
        }
        if (0) {
            using namespace where_;
            using T1 = operator_list<operator_::OR>;
            using T2 = oper_::append<T1, operator_::AND>::Result;
            using T3 = oper_::reverse<T2>::Result;
            static_assert(T3::Head == operator_::AND, "");
            static_assert(T3::Tail::Head == operator_::OR, "");
        }
    }
};
static unit_test s_test;
} // sample
} // make
} // db
} // sdl
#endif //#if SV_DEBUG

/*tab.scan([](auto row) {});
tab.select({ if row return true; });
tab.find_pk(int[]);
tab.find_less(int, use_indexes);
tab.find_less(300).select( {bool} );
tab.find_less(300).top(10); // last(10)
tab.top(5).union( tab.last(5) );
tab.find( {bool} ) 
*/
