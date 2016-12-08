// maketable_base.h
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_BASE_H__
#define __SDL_SYSTEM_MAKETABLE_BASE_H__

#include "dataserver/system/database.h"
#include "dataserver/maketable/maketable_meta.h"
#include "dataserver/system/type_utf.h"

namespace sdl { namespace db { namespace make {

class _make_base_table: public noncopyable {
    database const * const m_db;
    shared_usertable const m_schema;
    datatable const _datatable;
protected:
    _make_base_table(database const * p, shared_usertable const & s)
        : m_db(p), m_schema(s), _datatable(p, s)
    {
        SDL_ASSERT(m_db && m_schema);
    }
    ~_make_base_table() = default;
protected:
    template<class T> // T = col::
    using ret_type = typename T::ret_type;

    template<class T> // T = col::
    using val_type = typename T::val_type;

    template<class R, class V>
    static R get_empty(identity<R>, identity<V>, meta::is_fixed<true>) {
        static const V val{};
        return val;
    }
    template<class R, class V>
    static R get_empty(identity<R>, identity<V>, meta::is_fixed<false>) {
        A_STATIC_ASSERT_TYPE(R, V); // expect geo_mem or var_mem_t
        return {};
    }
    template<class T> // T = col::
    static ret_type<T> get_empty() {
        using R = ret_type<T>;
        using V = val_type<T>;
        return get_empty(identity<R>(), identity<V>(), meta::is_fixed<T::fixed>());
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
    database const * get_db() const { return m_db; } // for make_query
    datatable const & get_table() const { return _datatable; } // for make_query
};

template<class this_table, class record_type>
class base_access: noncopyable {
    using head_iterator = datatable::head_iterator;
    this_table const * const table;
public:
    using iterator = forward_iterator<base_access const, head_iterator>;
    explicit base_access(this_table const * const p): table(p) {
        SDL_ASSERT(table);
    }
    iterator begin() const {
        return iterator(this, table->get_table()._head.begin());
    }
    iterator end() const {
        return iterator(this, table->get_table()._head.end());
    }
private:
    friend iterator;
    record_type dereference(head_iterator const & it) const {
        A_STATIC_ASSERT_TYPE(row_head const *, remove_reference_t<decltype(*it)>);
        return record_type(table, *it);
    }
    static void load_next(head_iterator & it) {
        ++it;
    }
};

namespace make_query_ { 
    struct record_sort_impl;
}

template<class META>
class make_base_table: public _make_base_table {
    using TYPE_LIST = typename META::type_list;
public:
    enum { col_size = TL::Length<TYPE_LIST>::value };
    enum { col_fixed = meta::IsFixed<TYPE_LIST>::value };
protected:
    make_base_table(database const * p, shared_usertable const & s): _make_base_table(p, s) {}
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
        null_record(row_head const * h) noexcept : row(h) {
            SDL_ASSERT(row && use_record());
        }
        null_record() = default;
        ~null_record() = default;
    public:
        row_head const * head() const {
            return this->row;
        }
        template<class T> // T = col::
        bool is_null(identity<T>) const {
            static_assert(col_index<T>::value != -1, "");
            return null_bitmap(this->row)[T::place];
        }
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
        bool use_record() const {
            return this->row ? this->row->use_record() : false;
        }
    };
    template<class this_table, bool is_fixed>
    class base_record_t;

    template<class this_table>
    class base_record_t<this_table, true> : public null_record {
    protected:
        base_record_t(this_table const *, row_head const * h) noexcept
            : null_record(h) {
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
        base_record_t(this_table const * p, row_head const * h) noexcept
            : null_record(h), table(p) {
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
    private: // col_fixed = false
        friend make_query_::record_sort_impl;
        void set_table(this_table const * p) {
            this->table = p;
        }
        this_table const * get_table() const {
            return this->table;
        }
    };
protected:
    template<class this_table>
    class base_record : public base_record_t<this_table, col_fixed> {
        using base = base_record_t<this_table, col_fixed>;
    protected:
        base_record(this_table const * p, row_head const * h) noexcept : base(p, h) {}
        ~base_record() = default;
        base_record() = default;
    public:
        using table_type = this_table;
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
    public:
        template<class T> // T = col::
        std::string type_col() const {
            return to_string::type(this->get_value(identity<T>()));
        }
        template<class T> // T = col::
        std::string type_col(identity<T>) const {
            return to_string::type(this->get_value(identity<T>()));
        }
    public:
        template<class T, bool is_trim> // T = col::
        std::string type_col_utf8(identity<T>, info::is_trim_const<is_trim>) const {
            return info::type_col_utf8_t<T::_scalartype>::type_col(
                this->get_value(identity<T>()),
                info::is_trim_const<is_trim>());
        }
        template<class T, bool is_trim> // T = col::
        std::wstring type_col_wide(identity<T>, info::is_trim_const<is_trim>) const {
            return info::type_col_wide_t<T::_scalartype>::type_col(
                this->get_value(identity<T>()),
                info::is_trim_const<is_trim>());
        }
    public:
        template<class T> // T = col::
        std::string type_col_utf8() const {
            return this->type_col_utf8(identity<T>(), info::is_trim_const<false>());
        }
        template<class T> // T = col::
        std::wstring type_col_wide() const {
            return this->type_col_wide(identity<T>(), info::is_trim_const<false>());
        }
        template<class T> // T = col::
        std::string trim_col_utf8() const {
            return this->type_col_utf8(identity<T>(), info::is_trim_const<true>());
        }
        template<class T> // T = col::
        std::wstring trim_col_wide() const {
            return this->type_col_wide(identity<T>(), info::is_trim_const<true>());
        }
    }; // base_record
}; // make_base_table

template<class META>
struct make_clustered: META {
    make_clustered() = delete;
    enum { index_size = TL::Length<typename META::type_list>::value };
    template<size_t i> using index_col = typename TL::TypeAt<typename META::type_list, i>::Result;
};

namespace maketable_ { // protection from unintended ADL

template<class key_type, class T = typename key_type::this_clustered>
inline bool operator < (key_type const & x, key_type const & y) {
    A_STATIC_ASSERT_NOT_TYPE(geo_mem, key_type);
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

} // maketable_

using namespace maketable_;

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_BASE_H__
