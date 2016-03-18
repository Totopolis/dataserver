// maketable.h
//
#ifndef __SDL_SYSTEM_MAKETABLE_H__
#define __SDL_SYSTEM_MAKETABLE_H__

#pragma once

#include "common/type_seq.h"
#include "common/static_type.h"
#include "page_info.h"
#include "database.h"

namespace sdl { namespace db { namespace make {
namespace meta {

template<bool PK, size_t id = 0, sortorder ord = sortorder::ASC>
struct key {
    enum { is_primary_key = PK };
    enum { subid = id };
    static const sortorder order = ord;
};
using key_true = key<true, 0, sortorder::ASC>;
using key_false = key<false, 0, sortorder::NONE>;

template<scalartype::type, int> struct value_type; 
template<> struct value_type<scalartype::t_int, 4> {
    using type = int;
    enum { fixed = 1 };
};  
template<> struct value_type<scalartype::t_bigint, 8> {
    using type = uint64;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_smallint, 2> {
    using type = uint16;
    enum { fixed = 1 };
}; 
template<> struct value_type<scalartype::t_float, 8> { 
    using type = double;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_real, 4> { 
    using type = float;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_smalldatetime, 4> { 
    using type = smalldatetime_t;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_uniqueidentifier, 16> { 
    using type = guid_t;
    enum { fixed = 1 };
};
template<> struct value_type<scalartype::t_numeric, 9> { 
    using type = char[9]; //FIXME: not implemented
    enum { fixed = 1 };
};
template<int len> 
struct value_type<scalartype::t_char, len> {
    using type = char[len];
    enum { fixed = 1 };
};
template<int len> 
struct value_type<scalartype::t_nchar, len> {
    using type = nchar_t[len];
    enum { fixed = 1 };
};
template<int len> 
struct value_type<scalartype::t_varchar, len> {
    using type = var_mem;
    enum { fixed = 0 };
};
template<int len> 
struct value_type<scalartype::t_text, len> {
    using type = var_mem;
    enum { fixed = 0 };
};
template<int len> 
struct value_type<scalartype::t_ntext, len> {
    using type = var_mem;
    enum { fixed = 0 };
};
/*template<> struct value_type<scalartype::t_geometry, -1> {
    using type = var_mem;
    enum { fixed = 0 };
};*/
template<> struct value_type<scalartype::t_geography, -1> {
    using type = var_mem;
    enum { fixed = 0 };
};

template <bool v> struct is_fixed { enum { value = v }; };
template <bool v> struct is_array { enum { value = v }; };

template <class TList> struct IsFixed;
template <> struct IsFixed<NullType> {
    enum { value = 1 };
};
template <class T, class U>
struct IsFixed< Typelist<T, U> > {
    enum { value = T::fixed && IsFixed<U>::value };
};

template<size_t off, scalartype::type _type, int len = -1, typename base_key = key_false>
struct col : base_key {
private:
    using traits = value_type<_type, len>;
    using T = typename traits::type;
    col() = delete;
public:
    using val_type = T;
    using ret_type = typename std::conditional<std::is_array<T>::value, T const &, T>::type;
    enum { fixed = traits::fixed };
    enum { offset = off };
    enum { length = len };
    static const scalartype::type type = _type;
    static void test() {
        static_assert(!fixed || (length > 0), "col::length");
        static_assert(!fixed || (std::is_array<T>::value ? 
            (length == sizeof(val_type)/sizeof(typename std::remove_extent<T>::type)) :
            (length == sizeof(val_type))), "col::val_type");
    }
};

template<class T, size_t off = 0>
struct index_col {
    using col = T;
    using type = typename col::val_type;
    enum { offset = off };
};

template<class TYPE_LIST, size_t i>
using index_type = typename TL::TypeAt<TYPE_LIST, i>::Result::type; // = index_col::type

} // meta

template<class META>
class make_base_table: public noncopyable {
    using TYPE_LIST = typename META::type_list;
    database * const m_db;
public:
    enum { col_size = TL::Length<TYPE_LIST>::value };
    enum { col_fixed = meta::IsFixed<TYPE_LIST>::value };
protected:
    explicit make_base_table(database * p) : m_db(p) {
        SDL_ASSERT(m_db);
    }
    ~make_base_table() = default;
private:
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
        return m_db->get_variable(p, T::offset, T::type);
    }
    template<class T> // T = col::
    using col_index = TL::IndexOf<TYPE_LIST, T>;

    template<size_t i> 
    using col_t = typename TL::TypeAt<TYPE_LIST, i>::Result;

    template<size_t i>
    using col_ret_type = typename TL::TypeAt<TYPE_LIST, i>::Result::ret_type;

    template<class T> // T = col::
    ret_type<T> get_value(row_head const * const p, identity<T>, meta::is_fixed<0>) const {
        if (null_bitmap(p)[col_index<T>::value]) {
            return get_empty<T>();
        }
        static_assert(!T::fixed, "");
        return fixed_val<T>(p, meta::is_fixed<T::fixed>());
    }
    template<class T> // T = col::
    static ret_type<T> get_value(row_head const * const p, identity<T>, meta::is_fixed<1>) {
        if (null_bitmap(p)[col_index<T>::value]) {
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
    public:
        bool is_null(size_t const i) const {   
            return null_bitmap(this->row)[i];
        }
        template<size_t i>
        bool is_null() const {
            static_assert(i < col_size, "");
            return is_null(i);
        }
        template<class T> // T = col::
        bool is_null() const {
            return is_null<col_index<T>::value>();
        }
        explicit operator bool() const {
            return this->row != nullptr;
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
        ret_type<T> val() const {
            return make_base_table::get_value(this->row, identity<T>(), meta::is_fixed<T::fixed>());
        }
    protected:
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

        template<class T> // T = col::
        ret_type<T> get_value(identity<T>) const {
            return table->get_value(this->row, identity<T>(), meta::is_fixed<T::fixed>());
        }
    public:
        template<class T> // T = col::
        ret_type<T> val() const {
            return get_value(identity<T>());
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
protected:
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
};

template<class T> struct cluster_key {
    using type = typename T::key_type;
};
template<> struct cluster_key<void> {
    using type = void;
};

template<class this_table, class record>
class make_query: noncopyable {
    using cluster_index = typename this_table::cluster_index;
    using cluster_key = typename cluster_key<cluster_index>::type; /*typename Select<
        std::is_same<void, cluster_index>::value, void,
        typename cluster_index::key_type>::Result;*/
    using record_range = std::vector<record>; // prototype
private:
    this_table & table;
public:
    explicit make_query(this_table * p) : table(*p) {
        static_assert(std::is_pod<std::conditional<
            std::is_same<void, cluster_key>::value, int,
            cluster_key>::type>::value, "");
    }
    template<class fun_type>
    void scan_if(fun_type fun) {
        for (auto p : table) {
            if (!fun(p)) {
                break;
            }
        }
    }
    template<class fun_type> //FIXME: range of tuple<> ?
    record_range select(fun_type fun) {
        record_range ret;
        for (auto p : table) {
            if (fun(p)) {
                ret.push_back(p);
            }
        }
        return ret;
    }
    template<class fun_type>
    record find(fun_type fun) {
        for (auto p : table) { // linear
            if (fun(p)) {
                return p;
            }
        }
        return {};
    }
    //FIXME: find with cluster_index, add static_assert(sizeof(cluster_index::key_type) == ...)
    //FIXME: add index_tree<cluster_index>
};

template<class META>
class base_cluster: public META {
    using TYPE_LIST = typename META::type_list;
    base_cluster() = delete;
public:
    enum { index_size = TL::Length<TYPE_LIST>::value };        
    template<size_t i> using index_col = typename TL::TypeAt<TYPE_LIST, i>::Result;
protected:
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
    template<size_t i> static 
    meta::index_type<TYPE_LIST, i> const & get_col(const void * const begin) {
        using T = meta::index_type<TYPE_LIST, i>;
        return * reinterpret_cast<T const *>(get_address<i>(begin));
    }
    template<size_t i> static 
    meta::index_type<TYPE_LIST, i> & set_col(void * const begin) {
        using T = meta::index_type<TYPE_LIST, i>;
        return * reinterpret_cast<T *>(set_address<i>(begin));
    }
};

namespace sample {
struct dbo_META {
    struct col {
        struct Id : meta::col<0, scalartype::t_int, 4, meta::key<true, 0, sortorder::ASC>> { static const char * name() { return "Id"; } };
        struct Id2 : meta::col<4, scalartype::t_bigint, 8, meta::key<true, 1, sortorder::ASC>> { static const char * name() { return "Id2"; } };
        struct Col1 : meta::col<12, scalartype::t_char, 255> { static const char * name() { return "Col1"; } };
    };
    typedef TL::Seq<
        col::Id
        ,col::Id2
        ,col::Col1
    >::Type type_list;
    struct cluster_META {
        using T0 = meta::index_col<col::Id>;
        using T1 = meta::index_col<col::Id2, T0::offset + sizeof(T0::type)>;
        typedef TL::Seq<T0, T1>::Type type_list;
    };
    class cluster_index : public base_cluster<cluster_META> {
        using base = base_cluster<cluster_META>;
    public:
#pragma pack(push, 1)
        struct key_type {
            T0::type _0;
            T1::type _1;
            template<size_t i>
            auto get() const -> decltype(base::get_col<i>(nullptr)) {
                return base::get_col<i>(this);
            }
            template<size_t i>
            auto set() -> decltype(base::set_col<i>(nullptr)) {
                return base::set_col<i>(this);
            }
        };
#pragma pack(pop)
        friend key_type;
        static const char * name() { return ""; }
    };
    static const char * name() { return ""; }
    static const int32 id = 0;
};
class dbo_table : public dbo_META, public make_base_table<dbo_META> {
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
    explicit dbo_table(database * p, shared_usertable const & s)
        : base_table(p), _record(this, p, s) {}
    iterator begin() { return _record.begin(); }
    iterator end() { return _record.end(); }
    record::query query{ this };
    record::query * operator ->() { return &query; } // maybe
private:
    record::access _record;
};
} // sample
} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_H__
