// maketable_base.h
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_BASE_H__
#define __SDL_SYSTEM_MAKETABLE_BASE_H__

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

template<class META>
struct make_clustered: META {
    make_clustered() = delete;
    enum { index_size = TL::Length<typename META::type_list>::value };
    template<size_t i> 
    using index_col = typename TL::TypeAt<typename META::type_list, i>::Result;
};

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_BASE_H__
