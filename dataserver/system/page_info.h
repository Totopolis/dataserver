// page_info.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_INFO_H__
#define __SDL_SYSTEM_PAGE_INFO_H__

#include "page_head.h"
#include "spatial/spatial_type.h"
#include "spatial/geography.h"

namespace sdl { namespace db {

struct to_string: is_static {

    enum class type_format { less, more };

    static const char * type_name(pageType); 
    static const char * type_name(pageType::type); 
    static const char * type_name(dataType);
    static const char * type_name(dataType::type);
    static const char * type_name(recordType);
    static const char * type_name(pfs_full);
    static const char * type_name(sortorder);
    static const char * type_name(spatial_type);
    static const char * type_name(geometry_types);
    static const char * obj_name(obj_code);

    static std::string type(pageType);
    static std::string type(dataType);
    static std::string type(idxtype);
    static std::string type(idxstatus);
    static std::string type(uint8);
    static std::string type(guid_t const &);
    static std::string type(pageLSN const &);
    static std::string type(pageFileID const &);
    static std::string type(pageFileID const &, type_format);
    static std::string type(pageXdesID const &);
    static std::string type(datetime_t const &);
    static std::string type(smalldatetime_t);
    static std::string type(char const * buf, size_t buf_size);
    static std::string type(nchar_t const * buf, size_t buf_size);
    static std::string type(slot_array const &);
    static std::string type(null_bitmap const &);
    static std::string type(variable_array const &);
    static std::string type(auid_t const &);
    static std::string type(bitmask8);
    static std::string type(numeric9);
    static std::string type(pfs_byte);
    static std::string type(obj_code const &);
    static std::string type(overflow_page const &);
    static std::string type(text_pointer const &);
    static std::string type(recordID const &);
    static std::string type(schobj_id id) { return to_string::type(id._32); }
    static std::string type(nsid_id id) { return to_string::type(id._32); }
    static std::string type(index_id id) { return to_string::type(id._32); }
    static std::string type(column_xtype id) { return to_string::type(id._8); }
    static std::string type(column_id id) { return to_string::type(id._32); }
    static std::string type(iscolstatus s) { return to_string::type(s._32); }
    static std::string type(scalarlen len) { return to_string::type(len._16); }
    static std::string type(scalartype);
    static std::string type(spatial_cell const &, type_format);
    static std::string type(spatial_cell const &);
    static std::string type(spatial_point const &);
    static std::string type(geo_point const &);
    static std::string type(geo_pointarray const &);
    static std::string type(geo_linesegment const &);
    static std::string type(geo_mem const &);

    template<class T>
    static std::string type_less(T const & v) {
        return type(v, type_format::less);
    }
    template<size_t buf_size>
    static std::string type(char const(&buf)[buf_size]) {
        return type(buf, buf_size);
    }
    template<size_t buf_size>
    static std::string type(nchar_t const(&buf)[buf_size]) {
        return type(buf, buf_size);
    }
    template<int size>
    static std::string type(numeric_t<size> const & v) {
        return dump_mem(&v, sizeof(v));
    }
    static std::string type_raw(char const * buf, size_t buf_size);
    static std::string type_raw(char const * buf, size_t buf_size, type_format);

    static std::string type_raw(mem_range_t const & p) {
        SDL_ASSERT(p.first <= p.second);
        return type_raw(p.first, p.second - p.first);
    }

    template<size_t buf_size>
    static std::string type_raw(char const(&buf)[buf_size]) {
        return type_raw(buf, buf_size);
    }

    template<size_t buf_size>
    static std::string type_raw(char const(&buf)[buf_size], type_format f) {
        return type_raw(buf, buf_size, f);
    }

    static std::string type(nchar_range const & buf,
        type_format = type_format::less);

    static std::string dump_mem(void const * _buf, size_t const buf_size);

    template<class T> 
    static std::string dump_mem_t(T const & v) {
        return dump_mem(&v, sizeof(v));
    }
    static std::string dump_mem(mem_range_t const & m) {
        return dump_mem(m.first, mem_size(m));
    }
    static std::string dump_mem(vector_mem_range_t const &);
    static std::string dump_mem(var_mem const &);

    static std::string type_nchar(row_head const &, size_t col_index,
        type_format = type_format::less);

    static std::string type(pageFileID const * pages, size_t page_size);

    template<size_t page_size>
    static std::string type(pageFileID const(&pages)[page_size]) {
        return type(pages, page_size);
    }

    static std::string make_text(var_mem const &);
    static std::string make_ntext(var_mem const &);

    static std::string make_text(vector_mem_range_t const &);
    static std::string make_ntext(vector_mem_range_t const &);

    static guid_t parse_guid(std::string const &);
    static guid_t parse_guid(std::stringstream &);

    static int precision();
    static void precision(int);

    class stringstream : noncopyable { // format with precision
        std::stringstream ss;
    public:
        stringstream() = default;
        template<typename T>
        stringstream & operator << (T const & value) {
            ss << value;
            return *this;
        }
        stringstream & operator << (double);
        std::string str() const {
            return ss.str();
        }
    };

    template <class T>
    static std::string type(T const & value) {
        to_string::stringstream ss;
        ss << value;
        return ss.str();
    }
    static std::string type(var_mem const & v) {
        return dump_mem(v);
    }
    static std::string type(var_mem_t<scalartype::t_text> const & v) {
        return make_text(v);
    }
    static std::string type(var_mem_t<scalartype::t_ntext> const & v) {
        return make_ntext(v);
    }
    static std::string type(var_mem_t<scalartype::t_varchar> const & v) {
        return make_text(v);
    }
    static std::string type(var_mem_t<scalartype::t_nvarchar> const & v) {
        return make_ntext(v);
    }
    static std::string trim(std::string &&); // remove leading and trailing spaces
    static std::wstring trim(std::wstring &&); // remove leading and trailing spaces
};

inline std::ostream & operator <<(std::ostream & out, guid_t const & g) {
    out << to_string::type(g);
    return out;
}

inline std::stringstream & operator >>(std::stringstream & in, guid_t & g) {
    g = to_string::parse_guid(in);
    return in;
}

/*template <class T>
struct to_string_t : is_static {
    static std::string type(T const & value) {
        return to_string::type(value);
    }
};*/

struct page_info: is_static {
    static std::string type_meta(page_head const &);
    static std::string type_raw(page_head const &);
    static std::string type_meta(row_head const &);
};

namespace impl {

    template <bool v> struct variable
    {
        enum { value = v };
    };

    template <class type_list> struct processor;

    template <> struct processor<NullType>
    {
        template<class stream_type, class data_type, class format>
        static void print(stream_type &, data_type const * const, format){}
    };

    template <typename T, class format>
    struct parser
    {
        template<class stream_type, class data_type>
        static void apply(stream_type & ss, data_type const * const data, variable<false>)
        {
            static_assert(!T::variable, "");
            typedef typename T::type value_type;
            char const * const p = reinterpret_cast<char const *>(data) + T::offset;
            value_type const & value = *reinterpret_cast<value_type const *>(p);
            ss << "0x" << std::uppercase << std::hex << T::offset << ": " << std::dec;
            ss << T::name() << " = ";
            ss << format::type::type(value);
            ss << std::endl;
        }

        template<class stream_type, class data_type>
        static void apply(stream_type & ss, data_type const * const data, variable<true>)
        {
            static_assert(T::variable, "");
            ss << "\nvar_" << T::offset << ":\n";
            ss << T::name() << " = ";
            ss << format::type::type(data, identity<T>());
            ss << std::endl;
        }
    };

    template <class T, class U> // T = meta::col_type
    struct processor< Typelist<T, U> >
    {
        template<class stream_type, class data_type, class format = identity<to_string> >
        static void print(stream_type & ss, data_type const * const data, format f = format())
        {
            parser<T, format>::apply(ss, data, variable<T::variable>());
            processor<U>::print(ss, data, f);
        }
    };

namespace algorithm {

    template <class type_list> struct for_each;

    template <> struct for_each<NullType>
    {
        template<class data_type, class fun_type>
        static void apply(data_type const * const, fun_type){}
    };

    template <typename T> // T = meta::col_type
    struct row_value
    {
        template<class row_type> static 
        typename T::type const & result(row_type const * const row, variable<false>)
        {
            static_assert(!T::variable, "");
            typedef typename T::type value_type;
            char const * const p = reinterpret_cast<char const *>(row) + T::offset;
            return *reinterpret_cast<value_type const *>(p);
        }

        template<class row_type> static 
        std::string result(row_type const * const row, variable<true>)
        {
            static_assert(T::variable, "");
            static_assert(std::is_same<typename T::type, nchar_range>::value, ""); // supported type
            static_assert(null_bitmap_traits<row_type>::value, "");
            static_assert(variable_array_traits<row_type>::value, "");
            return to_string::type_nchar(row->data.head, T::offset,
                to_string::type_format::less);
        }
    };

    template <class T, class U> // T = meta::col_type
    struct for_each< Typelist<T, U> >
    {
        template<class data_type, class fun_type>
        static void apply(data_type const * const data, fun_type fun)
        {
            fun(row_value<T>::result(data, variable<T::variable>()), identity<T>());
            for_each<U>::apply(data, fun);
        }
    };

} // algorithm
} // impl

struct to_string_with_head : to_string {
    using to_string::type; // allow type() methods from base class
    static std::string type(row_head const &);

    template<class row_type, class col_type>
    static std::string type(row_type const * row, identity<col_type>) {
        static_assert(std::is_same<typename col_type::type, nchar_range>::value, "nchar_range"); // supported type
        static_assert(null_bitmap_traits<row_type>::value, "null_bitmap");
        static_assert(variable_array_traits<row_type>::value, "variable_array");
        return to_string::type_nchar(
            row->data.head,
            col_type::offset,
            to_string::type_format::more);
    }
};

template<class T>
struct get_type_list : is_static {
    using type = typename T::meta::type_list;
};

template< class T >
using get_type_list_t = typename get_type_list<T>::type;

struct processor_row: is_static
{
    template<class row_type>
    static std::string type_meta(row_type const & data) {
        using type_list = get_type_list_t<row_type>;
        std::stringstream ss;
        impl::processor<type_list>::print(ss, &data, identity<to_string_with_head>());
        return ss.str();
    }
};

struct for_each_row : is_static
{
    template<class row_type, class fun_type>
    static void apply(row_type const & data, fun_type fun) {
        using type_list = get_type_list_t<row_type>;
        impl::algorithm::for_each<type_list>::apply(&data, fun);
    }
};

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_INFO_H__