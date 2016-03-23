// maketable.h
//
#ifndef __SDL_SYSTEM_MAKETABLE_H__
#define __SDL_SYSTEM_MAKETABLE_H__

#pragma once

#include "database.h"
#include "maketable_meta.h"

namespace sdl { namespace db { namespace make {

class _make_base_table: public noncopyable {
    database * const m_db;
    shared_usertable const m_schema;
protected:
    _make_base_table(database * p, shared_usertable const & s): m_db(p), m_schema(s) {
        SDL_ASSERT(m_db && m_schema);
    }
    ~_make_base_table() = default;
protected:
    template<class T> // T = col::
    using ret_type = typename T::ret_type;

    template<class T> // T = col::
    using val_type = typename T::val_type;

    template<class R, class V>
    static R get_empty(identity<R>, identity<V>) {
        static const V val{};
        return val;
    }
    static var_mem get_empty(identity<var_mem>, identity<var_mem>) {
        return {};
    }
    template<class T> // T = col::
    static ret_type<T> get_empty() {
        using R = ret_type<T>;
        using V = val_type<T>;
        return get_empty(identity<R>(), identity<V>());
    }
    template<class T> // T = col::
    static ret_type<T> fixed_val(row_head const * const p, meta::is_fixed<1>) { // is fixed 
        static_assert(T::fixed, "");
        return p->fixed_val<typename T::val_type>(T::offset);
    }
    template<class T> // T = col::
    ret_type<T> fixed_val(row_head const * const p, meta::is_fixed<0>) const { // is variable 
        static_assert(!T::fixed, "");
        return m_db->var_data(p, T::offset, T::type);
    }
public:
    database * get_db() const { return m_db; } // for make_query
};

template<class this_table, class record_type>
class base_access: noncopyable {
    using record_iterator = datatable::datarow_iterator;
    this_table const * const table;
    detached_datarow _datarow;
public:
    using iterator = forward_iterator<base_access, record_iterator>;
    base_access(this_table const * p, database * const d, shared_usertable const & s)
        : table(p), _datarow(d, s)
    {
        SDL_ASSERT(table);
        SDL_ASSERT(s->get_id() == this_table::id);
        SDL_ASSERT(s->name() == this_table::name());
    }
    iterator begin() {
        return iterator(this, _datarow.begin());
    }
    iterator end() {
        return iterator(this, _datarow.end());
    }
private:
    friend iterator;
    record_type dereference(record_iterator const & it) {
        A_STATIC_CHECK_TYPE(row_head const *, *it);
        return record_type(table, *it);
    }
    void load_next(record_iterator & it) {
        ++it;
    }
};

template<class META>
class make_base_table: public _make_base_table {
    using TYPE_LIST = typename META::type_list;
public:
    enum { col_size = TL::Length<TYPE_LIST>::value };
    enum { col_fixed = meta::IsFixed<TYPE_LIST>::value };
protected:
    make_base_table(database * p, shared_usertable const & s): _make_base_table(p, s) {}
    ~make_base_table() = default;
private:
    template<class T> // T = col::
    using col_index = TL::IndexOf<TYPE_LIST, T>;

    template<size_t i> 
    using col_t = typename TL::TypeAt<TYPE_LIST, i>::Result;

    template<size_t i>
    using col_ret_type = typename TL::TypeAt<TYPE_LIST, i>::Result::ret_type;

    template<class T> // T = col::
    ret_type<T> get_value(row_head const * const p, identity<T>, meta::is_fixed<0>) const {
        if (null_bitmap(p)[T::place]) {
            return get_empty<T>();
        }
        static_assert(!T::fixed, "");
        return fixed_val<T>(p, meta::is_fixed<T::fixed>());
    }
    template<class T> // T = col::
    static ret_type<T> get_value(row_head const * const p, identity<T>, meta::is_fixed<1>) {
        if (null_bitmap(p)[T::place]) {
            return get_empty<T>();
        }
        static_assert(T::fixed, "");
        return fixed_val<T>(p, meta::is_fixed<T::fixed>());
    }
private:
    class null_record {
    protected:
        row_head const * row = nullptr;
        null_record(row_head const * h): row(h) {
            SDL_ASSERT(row);
        }
        null_record() = default;
        ~null_record() = default;

        template<class T> // T = col::
        bool is_null(identity<T>) const {
            static_assert(col_index<T>::value != -1, "");
            return null_bitmap(this->row)[T::place];
        }
    public:
        template<class T> // T = col::
        bool is_null() const {
            return is_null(identity<T>());
        }
        template<size_t i>
        bool is_null() const {
            static_assert(i < col_size, "");
            return is_null(identity<col_t<i>>());
        }
        explicit operator bool() const {
            return this->row != nullptr;
        }
        bool is_same(const null_record & src) const {
            return (this->row == src.row);
        }
    };
    template<class this_table, bool is_fixed>
    class base_record_t;

    template<class this_table>
    class base_record_t<this_table, true> : public null_record {
    protected:
        base_record_t(this_table const *, row_head const * h): null_record(h) {
            static_assert(col_fixed, "");
        }
        base_record_t() = default;
        ~base_record_t() = default;
    public:
        template<class T> // T = col::
        ret_type<T> get_value(identity<T>) const {
            return make_base_table::get_value(this->row, identity<T>(), meta::is_fixed<T::fixed>());
        }
    };
    template<class this_table>
    class base_record_t<this_table, false> : public null_record {
        this_table const * table = nullptr;
    protected:
        base_record_t(this_table const * p, row_head const * h): null_record(h), table(p) {
            SDL_ASSERT(table);
            static_assert(!col_fixed, "");
        }
        base_record_t() = default;
        ~base_record_t() = default;
    protected:
        template<class T> // T = col::
        ret_type<T> get_value(identity<T>) const {
            return table->get_value(this->row, identity<T>(), meta::is_fixed<T::fixed>());
        }
    };
protected:
    template<class this_table>
    class base_record : public base_record_t<this_table, col_fixed> {
        using base = base_record_t<this_table, col_fixed>;
    protected:
        base_record(this_table const * p, row_head const * h): base(p, h) {}
        ~base_record() = default;
        base_record() = default;
    public:
        template<class T> // T = col::
        ret_type<T> val() const {
            return this->get_value(identity<T>());
        }
        template<class T> // T = col::
        ret_type<T> val(identity<T>) const {
            return this->get_value(identity<T>());
        }
        template<size_t i>
        col_ret_type<i> get() const {
            static_assert(i < col_size, "");
            return this->get_value(identity<col_t<i>>());
        }
        template<class T> // T = col::
        std::string type_col() const {
            return type_col<T>(meta::is_fixed<T::fixed>());
        }
    private:
        template<class T> // T = col::
        std::string type_col(meta::is_fixed<1>) const {
            return to_string::type(this->get_value(identity<T>()));
        }
        template<class T> // T = col::
        std::string type_col(meta::is_fixed<0>) const {
            return to_string::dump_mem(this->get_value(identity<T>()));
        }
    };
}; // make_base_table

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
        if (row_head const * head = _datatable.find_row_head_t(key)) { // not optimized
            return record(&m_table, head);
        }
        return {};
    }
    record static_find_with_index(key_type const & key) {
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
    }
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

template<class META>
struct make_clustered: META {
    make_clustered() = delete;
    enum { index_size = TL::Length<typename META::type_list>::value };        
    template<size_t i> using index_col = typename TL::TypeAt<typename META::type_list, i>::Result;
};

#if SDL_DEBUG
namespace sample {
struct dbo_META {
    struct col {
        struct Id : meta::col<0, 0, scalartype::t_int, 4, meta::key<true, 0, sortorder::ASC>> { static const char * name() { return "Id"; } };
        struct Id2 : meta::col<1, 4, scalartype::t_bigint, 8, meta::key<true, 1, sortorder::ASC>> { static const char * name() { return "Id2"; } };
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
        };
#pragma pack(pop)
        static const char * name() { return ""; }
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
