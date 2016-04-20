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

//------------------------------------------------------------------------------

struct ignore_index {};
struct use_index {};
struct unique_false {};
struct unique_true {};

template<typename col_type> 
struct where {
    using col = col_type;
    using val_type = typename col_type::val_type;
    static const char * name() { return col::name(); }
    val_type const & value;
    where(val_type const & v): value(v) {}
};

namespace where_ { //FIXME: prototype

template<class T, sortorder ord = sortorder::ASC> 
struct ORDER_BY{};

template<class T> // T = col::
struct WHERE {
    WHERE(std::initializer_list<typename T::val_type>){}
};

template<class T>
struct IN {
    IN(std::initializer_list<typename T::val_type>){}
};

template<class T>
struct NOT {
    NOT(std::initializer_list<typename T::val_type>){}
};

template<class T>
struct LESS {
    LESS(std::initializer_list<typename T::val_type>){}
};

template<class T>
struct GREATER {
    GREATER(std::initializer_list<typename T::val_type>){}
};

template<class T>
struct LESS_EQ {
    LESS_EQ(std::initializer_list<typename T::val_type>){}
};

template<class T>
struct GREATER_EQ {
    GREATER_EQ(std::initializer_list<typename T::val_type>){}
};

template<class T>
struct BETWEEN {
    BETWEEN(std::initializer_list<typename T::val_type>){}
    BETWEEN(typename T::val_type, typename T::val_type){}
};

enum class operator_ { OR, AND };

template <operator_ T, class U = NullType>
struct operator_list {
    static const operator_ Head = T;
    typedef U Tail;
};

template <class TList, operator_ T> struct append;

template <operator_ T> 
struct append<NullType, T>
{
    using Result = operator_list<T>;
};

template <operator_ Head, class Tail, operator_ T>
struct append<operator_list<Head, Tail>, T>
{
    using Result = operator_list<Head, typename append<Tail, T>::Result>;
};

} //where_

template<class this_table, class record>
class make_query: noncopyable {
    using table_clustered = typename this_table::clustered;
    using key_type = meta::cluster_key<table_clustered, NullType>;
    using KEY_TYPE_LIST = meta::cluster_type_list<table_clustered, NullType>;
    enum { index_size = meta::cluster_index_size<table_clustered>::value };
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
    template<class T> // T = meta::index_col
    using key_index = TL::IndexOf<KEY_TYPE_LIST, T>;

    template<size_t i>
    using key_index_at = typename TL::TypeAt<KEY_TYPE_LIST, i>::Result;

    class read_key_fun {
        key_type & dest;
        record const & src;
    public:
        read_key_fun(key_type & d, record const & s) : dest(d), src(s) {}
        template<class T> // T = meta::index_col
        void operator()(identity<T>) const {
            enum { set_index = key_index<T>::value };
            A_STATIC_ASSERT_IS_POD(typename T::type);
            meta::copy(dest.set(Int2Type<set_index>()), src.val(identity<typename T::col>()));
        }
    };
    class select_key_list  {
        const key_type * _First;
        const key_type * _Last;
    public:
        select_key_list(std::initializer_list<key_type> in) : _First(in.begin()), _Last(in.end()) {}
        select_key_list(key_type const & in) : _First(&in), _Last(&in + 1){}
        select_key_list(std::vector<key_type> const & in) : _First(in.data()), _Last(in.data() + in.size()){}
        const key_type * begin() const {
            return _First;
        }
        const key_type * end() const {
            return _Last;
        }
        size_t size() const {
            return ((size_t)(_Last - _First));
        }
    };
    template<size_t i> static void set_key(key_type &) {}
    template<size_t i, typename T, typename... Ts>
    static void set_key(key_type & dest, T && value, Ts&&... params) {
        dest.set(Int2Type<i>()) = std::forward<T>(value);
        set_key<i+1>(dest, params...);
    }
public:
    static key_type read_key(record const & src) {
        key_type dest; // uninitialized
        meta::processor<KEY_TYPE_LIST>::apply(read_key_fun(dest, src));
        return dest;
    }
    key_type read_key(row_head const * h) const {
        SDL_ASSERT(h);
        return make_query::read_key(record(&m_table, h));
    }
    template<typename... Ts> static
    key_type make_key(Ts&&... params) {
        static_assert(index_size == sizeof...(params), "make_key"); 
        key_type dest; // uninitialized
        set_key<0>(dest, params...);
        return dest;
    }
private:
    record_range select(select_key_list, ignore_index, unique_false);
    record_range select(select_key_list, ignore_index, unique_true);
    record_range select(select_key_list, use_index, unique_true);
    static void select_n() {}
public:
    template<class T1 = use_index, class T2 = unique_true>
    record_range select(select_key_list in) {
        return select(in, T1(), T2());
    }
    //record_range select(select_key_list, enum_index = enum_index::use_index, enum_unique = enum_unique::unique_true);    
    //FIXME: SELECT * WHERE id = 1|2|3 USE|IGNORE INDEX
    //FIXME: SELECT select_list [ ORDER BY ] [USE INDEX or IGNORE INDEX]
    //FIXME: select * from GeoTable as gt where myPoint.STDistance(gt.Geo) <= 50

    template<typename T, typename... Ts> 
    void select_n(where<T> col, Ts const & ... params) {
        enum { col_found = TL::IndexOf<typename this_table::type_list, T>::value };
        enum { key_found = meta::cluster_col_find<KEY_TYPE_LIST, T>::value };
        static_assert(col_found != -1, "");
        using type_list = typename TL::Seq<T, Ts...>::Type; // test
        static_assert(TL::Length<type_list>::value == sizeof...(params) + 1, "");
        SDL_ASSERT(where<T>::name() == T::name()); // same memory
        SDL_TRACE(
            "col:", col_found, 
            " key:", key_found, 
            " name:", T::name(),
            " value:", col.value);
        select_n(params...);
    }
private:
    using operator_ = where_::operator_;

    template<class TList, class OPER>
    struct sub_expr {

        using type_list = TList;
        using oper_list = OPER;

        template<class T, operator_ op>
        using ret_expr = typename sub_expr<
            typename TL::Append<type_list, T>::Result, 
            where_::operator_list<op>
            >;

        template<class T>
        ret_expr<T, operator_::OR> && operator | (T const &) {
            return {};
        }
        template<class T>
        ret_expr<T, operator_::AND> && operator && (T const &) {
            return {};
        }
        record_range VALUES() {
            SDL_TRACE("\nVALUES:");
            meta::trace_typelist<type_list>();
            return {};
        }
        operator record_range() { return VALUES(); }
    };
    class select_expr : noncopyable {
    
        template<class T, operator_ op>
        using ret_expr = typename sub_expr<
            typename TL::Seq<T>::Type,
            where_::operator_list<op>
            >;
    public:
        template<class T>
        ret_expr<T, operator_::OR> && operator | (T const &) {
            return {};
        }
        template<class T>
        ret_expr<T, operator_::AND> && operator && (T const &) {
            return {};
        }
    };
public:
    select_expr SELECT;
};

} // make
} // db
} // sdl

#include "maketable.hpp"

#endif // __SDL_SYSTEM_MAKETABLE_H__
