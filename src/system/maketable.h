// maketable.h
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_H__
#define __SDL_SYSTEM_MAKETABLE_H__

#include "maketable_base.h"
#include "index_tree_t.h"

namespace sdl { namespace db { namespace make {

template<class key_type>
inline bool operator < (key_type const & x, key_type const & y) {
    return key_type::this_clustered::is_less(x, y);
}

template<class key_type, class T = typename key_type::this_clustered>
inline bool operator == (key_type const & x, key_type const & y) {
    A_STATIC_ASSERT_NOT_TYPE(void, T);
    return !((x < y) || (y < x));
}
template<class key_type, class T = typename key_type::this_clustered>
inline bool operator != (key_type const & x, key_type const & y) {
    A_STATIC_ASSERT_NOT_TYPE(void, T);
    return !(x == y);
}

enum enum_index { ignore_index, use_index };
template<enum_index v> using enum_index_t = Val2Type<enum_index, v>;

enum enum_unique { unique_false, unique_true };
template<enum_unique v> using enum_unique_t = Val2Type<enum_unique, v>;

template<class this_table, class record>
class make_query: noncopyable {
    using table_clustered = typename this_table::clustered;
    using key_type = meta::cluster_key_t<table_clustered, NullType>;
    enum { index_size = table_clustered::index_size };
private:
    this_table & m_table;
    page_head const * const m_cluster;
public:
    using record_range = std::vector<record>;
    make_query(this_table * p, database * const d, shared_usertable const & s)
        : m_table(*p)
        , m_cluster(d->get_cluster_root(_schobj_id(this_table::id)))
    {
        SDL_ASSERT(meta::test_clustered<table_clustered>());
    }
    template<class fun_type>
    void scan_if(fun_type fun) {
        for (auto p : m_table) {
            A_STATIC_CHECK_TYPE(record, p);
            if (!fun(p)) {
                break;
            }
        }
    }
    template<class fun_type>
    record find(fun_type fun) {
        for (auto p : m_table) { // linear search
            A_STATIC_CHECK_TYPE(record, p);
            if (fun(p)) {
                return p;
            }
        }
        return {};
    }
    template<typename... Ts>
    record find_with_index_n(Ts&&... params) {
        static_assert(index_size == sizeof...(params), ""); 
        return find_with_index(key_type{params...});  
    }
    record find_with_index(key_type const &);
private:
    class read_key_fun {
        key_type & dest;
        record const & src;
    public:
        read_key_fun(key_type & d, record const & s) : dest(d), src(s) {}
        template<class T> // T = meta::index_col
        void operator()(identity<T>) const {
            enum { set_i = TL::IndexOf<typename table_clustered::type_list, T>::value };
            A_STATIC_ASSERT_IS_POD(typename T::type);
            meta::copy(dest.set(Int2Type<set_i>()), src.val(identity<typename T::col>()));
        }
    };
    class select_list : public std::initializer_list<key_type> {
        using initializer_list = std::initializer_list<key_type>;
    public:
        select_list(initializer_list in) : initializer_list(in){}
        select_list(key_type const & in) : initializer_list(&in, (&in) + 1){}
        select_list(std::vector<key_type> const & in) : initializer_list(in.data(), in.data() + in.size()){}
    };
public:
    static key_type read_key(record const & src) {
        key_type dest; // uninitialized
        meta::processor<typename table_clustered::type_list>::apply(read_key_fun(dest, src));
        return dest;
    }
    key_type read_key(row_head const * h) const {
        SDL_ASSERT(h);
        return make_query::read_key(record(&m_table, h));
    }
    template<typename... Ts> static
    key_type make_key(Ts&&... params) {
        static_assert(index_size == sizeof...(params), "make_key"); 
        return key_type{params...};  
    }
    record_range select(select_list, 
        enum_index = enum_index::use_index, 
        enum_unique = enum_unique::unique_true);    
private:
    record_range select(select_list in, enum_index_t<ignore_index>, enum_unique_t<unique_false>);
    record_range select(select_list in, enum_index_t<ignore_index>, enum_unique_t<unique_true>);
    record_range select(select_list in, enum_index_t<use_index>, enum_unique_t<unique_true>);
public:
    template<enum_index v1, enum_unique v2> 
    record_range select(select_list in) {
        return select(in, enum_index_t<v1>(), enum_unique_t<v2>());
    }
    //FIXME: SELECT * WHERE id = 1|2|3 USE|IGNORE INDEX
    //FIXME: SELECT select_list [ ORDER BY ] [USE INDEX or IGNORE INDEX]
};

template<class this_table, class record>
record make_query<this_table, record>::find_with_index(key_type const & key) {
    if (m_cluster) {
        auto const db = m_table.get_db();
        if (auto const id = todo::index_tree<key_type>(db, m_cluster).find_page(key)) {
            if (page_head const * const h = db->load_page_head(id)) {
                SDL_ASSERT(h->is_data());
                const datapage data(h);
                if (!data.empty()) {
                    size_t const slot = data.lower_bound(
                        [this, &key](row_head const * const row, size_t) {
                        return (this->read_key(row) < key);
                    });
                    if (slot < data.size()) {
                        row_head const * const head = data[slot];
                        if (!(key < read_key(head))) {
                            return record(&m_table, head);
                        }
                    }
                }
            }
        }
    }
    return {};
}

template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_list in, enum_index_t<ignore_index>, enum_unique_t<unique_false>) {
    record_range result;
    if (in.size()) {
        result.reserve(in.size());
        for (auto p : m_table) { // scan all table
            A_STATIC_CHECK_TYPE(record, p);
            auto const key = make_query::read_key(p);
            for (auto const & k : in) {
                if (key == k) {
                    result.push_back(p);
                }
            }
        }
    }
    return result;              
}

template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_list in, enum_index_t<ignore_index>, enum_unique_t<unique_true>) {
    record_range result;
    if (in.size()) {
        result.reserve(in.size());
        std::vector<const key_type *> look(in.size());
        for (size_t i = 0; i < in.size(); ++i) {
            look[i] = in.begin() + i;
        }
        size_t size = in.size();
        for (auto p : m_table) {
            A_STATIC_CHECK_TYPE(record, p);
            auto const key = make_query::read_key(p);
            for (size_t i = 0; i < size; ++i) {
                if (key == *look[i]) {
                    result.push_back(p);
                    if (!(--size))
                        return result;
                    look[i] = look[size];
                }
            }
        }
    }
    return result; // not all keys found
}

template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_list in, enum_index_t<use_index>, enum_unique_t<unique_true>) {
    record_range result;
    if (in.size()) {
        result.reserve(in.size());
        for (auto const & key : in) {
            if (auto p = find_with_index(key)) { //FIXME: can be optimized (sort keys and use previous search result)
                result.push_back(p);
            }
        }
    }
    return result;
}

template<class this_table, class record>
typename make_query<this_table, record>::record_range
make_query<this_table, record>::select(select_list in, enum_index const v1, enum_unique const v2) {
    if (enum_index::ignore_index == v1) {
        if (enum_unique::unique_false == v2) {
            return select(in, enum_index_t<ignore_index>(), enum_unique_t<unique_false>());
        }
        SDL_ASSERT(enum_unique::unique_true == v2);
        return select(in, enum_index_t<ignore_index>(), enum_unique_t<unique_true>());
    }
    SDL_ASSERT(enum_index::use_index == v1);
    SDL_ASSERT(enum_unique::unique_true == v2);
    return select(in, enum_index_t<use_index>(), enum_unique_t<unique_true>());
}

#if SDL_DEBUG
namespace sample {
struct dbo_META {
    struct col {
        struct Id : meta::col<0, 0, scalartype::t_int, 4, meta::key<true, 0, sortorder::ASC>> { static const char * name() { return "Id"; } };
        struct Id2 : meta::col<1, 4, scalartype::t_bigint, 8, meta::key<true, 1, sortorder::DESC>> { static const char * name() { return "Id2"; } };
        struct Col1 : meta::col<2, 12, scalartype::t_char, 255> { static const char * name() { return "Col1"; } };
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
    struct clustered : make_clustered<clustered_META> {
#pragma pack(push, 1)
        struct key_type {
            T0::type _0;
            T1::type _1;
            T0::type const & get(Int2Type<0>) const { return _0; }
            T1::type const & get(Int2Type<1>) const { return _1; }
            T0::type & set(Int2Type<0>) { return _0; }
            T1::type & set(Int2Type<1>) { return _1; }
            template<size_t i> auto get() -> decltype(get(Int2Type<i>())) { return get(Int2Type<i>()); }
            template<size_t i> auto set() -> decltype(set(Int2Type<i>())) { return set(Int2Type<i>()); }
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
    };
    static const char * name() { return ""; }
    static const int32 id = 0;
};

class dbo_table final : public dbo_META, public make_base_table<dbo_META> {
    using base_table = make_base_table<dbo_META>;
    using this_table = dbo_table;
public:
    class record : public base_record<this_table> {
        using base = base_record<this_table>;
        using access = base_access<this_table, record>;
        using query = make_query<this_table, record>;
        friend access;
        friend query;
        friend this_table;
        record(this_table const * p, row_head const * h): base(p, h) {}
        record() = default;
    public:
        auto Id() const -> col::Id::ret_type { return val<col::Id>(); }
        auto Col1() const -> col::Col1::ret_type { return val<col::Col1>(); }
    };
public:
    using iterator = record::access::iterator;
    using query_type = record::query;
    explicit dbo_table(database * p, shared_usertable const & s)
        : base_table(p, s), _record(this, p, s), query(this, p, s)
    {}
    iterator begin() { return _record.begin(); }
    iterator end() { return _record.end(); }
    query_type * operator ->() { return &query; }
private:
    record::access _record;
    query_type query;
};
} // sample
#endif //#if SV_DEBUG

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_H__
