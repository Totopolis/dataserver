// maketable.h
//
#ifndef __SDL_SYSTEM_MAKETABLE_H__
#define __SDL_SYSTEM_MAKETABLE_H__

#pragma once

#include "maketable_base.h"
#include "index_tree_t.h"

namespace sdl { namespace db { namespace make {

template<class this_table, class record>
class make_query: noncopyable {
private:
    this_table & m_table;
    datatable _datatable; //FIXME: temporal
    shared_cluster_index const m_cluster;
private:
    using table_clustered = typename this_table::clustered;
    using key_type = meta::cluster_key_t<table_clustered, NullType>;
    using vector_record = std::vector<record>;
public:
    make_query(this_table * p, database * const d, shared_usertable const & s)
        : m_table(*p), _datatable(d, s)
        , m_cluster(d->get_cluster_index(schobj_id::init(this_table::id)))
    {
        SDL_ASSERT(meta::test_clustered<table_clustered>());
        SDL_ASSERT(!m_cluster || (m_cluster->get_id() == this_table::id));
    }
    template<class fun_type>
    void scan_if(fun_type fun) {
        for (auto p : m_table) {
            if (!fun(p)) {
                break;
            }
        }
    }
    //FIXME: SELECT select_list [ ORDER BY ] [USE INDEX or IGNORE INDEX]
    template<class fun_type>
    vector_record select(fun_type fun) {
        vector_record result;
        for (auto p : m_table) {
            if (fun(p)) {
                result.push_back(p);
            }
        }
        return result;
    }
    template<class fun_type>
    record find(fun_type fun) {
        for (auto p : m_table) { // linear
            if (fun(p)) {
                return p;
            }
        }
        return {};
    }
    template<typename... Ts>
    record find_with_index_n(Ts&&... params) {
        static_assert(table_clustered::index_size == sizeof...(params), ""); 
        return find_with_index(key_type{params...});  
    }
    record find_with_index(key_type const & key) {
        if (0 && m_cluster) {
            todo::index_tree<key_type> tree(m_table.get_db(), m_cluster->root());
        }
        else if (row_head const * head = _datatable.find_row_head_t(key)) { // not optimized
            return record(&m_table, head);
        }
        return {};
    }
    /*record static_find_with_index(key_type const & key) {
        if (0 && m_cluster) {
            database * const db = m_table.get_db();
            index_tree tree(db, m_cluster); //FIXME: index_tree<key_type> 
            if (auto const id = tree.find_page_t(key._0)) {
                if (page_head const * const h = db->load_page_head(id)) {
                    SDL_ASSERT(h->is_data());
                    const datapage data(h); //FIXME: read key_type from record
                    if (!data.empty()) {
                        size_t const slot = data.lower_bound([](row_head const * const row, size_t) {
                            return false;
                        });
                        if (slot < data.size()) {
                        }
                    }
                }
            }
        }
        return {};
    }*/
private:
    class read_key_fun {
        key_type * dest;
        record const * src;
    public:
        read_key_fun(key_type & d, record const & s) : dest(&d), src(&s) {}
        template<class T> // T = meta::index_col
        void operator()(identity<T>) const {
            enum { set_i = TL::IndexOf<typename table_clustered::type_list, T>::value };
            dest->set(Int2Type<set_i>()) = src->val(identity<typename T::col>());
        }
    };
public:
    static key_type read_key(record const & src) {
        key_type dest; // uninitialized
        meta::processor<typename table_clustered::type_list>::apply(read_key_fun(dest, src));
        return dest;
    }
};

template<class key_type>
inline bool operator < (key_type const & x, key_type const & y) {
    return key_type::this_clustered::is_less(x, y);
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
            return
                meta::is_less<T0>::less(x._0, y._0) &&
                meta::is_less<T1>::less(x._1, y._1);
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
