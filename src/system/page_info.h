// page_info.h
//
#ifndef __SDL_SYSTEM_PAGE_INFO_H__
#define __SDL_SYSTEM_PAGE_INFO_H__

#pragma once

#include "page_head.h"
#include <sstream>

namespace sdl { namespace db {

struct to_string: is_static {

    static const char * type_name(pageType); 
    static const char * type_name(dataType); 
    static const char * code_name(obj_code const &);
    static const char * type_name(pfs_full);

    template <class T>
    static std::string type(T const & value);

    static std::string type(pageType);
    static std::string type(dataType);
    static std::string type(uint8);
    static std::string type(guid_t const &);
    static std::string type(pageLSN const &);
    static std::string type(pageFileID const &);
    static std::string type(pageXdesID const &);
    static std::string type(datetime_t const &);
    static std::string type(nchar_t const * buf, size_t buf_size);
    static std::string type(slot_array const &);
    static std::string type(null_bitmap const &);
    static std::string type(variable_array const &);
    static std::string type(auid_t const &);
    static std::string type(bitmask8);
    static std::string type(pfs_byte);
    static std::string type(obj_code const &);
    static std::string type(overflow_page const &);
    static std::string type(text_pointer const &);
    static std::string type(schobj_id id) {
        return to_string::type(id._32);
    }

    template<size_t buf_size>
    static std::string type(nchar_t const(&buf)[buf_size]) {
        return type(buf, buf_size);
    }

    static std::string type_raw(char const * buf, size_t buf_size);

    static std::string type_raw(mem_range_t const & p) {
        SDL_ASSERT(p.first <= p.second);
        return type_raw(p.first, p.second - p.first);
    }

    template<size_t buf_size>
    static std::string type_raw(char const(&buf)[buf_size]) {
        return type_raw(buf, buf_size);
    }

    enum class nchar_format { less, more };
    static std::string type(nchar_range const & buf,
        nchar_format = nchar_format::less);

    static std::string dump_mem(void const * _buf, size_t const buf_size);

    static std::string type_nchar(row_head const &, size_t col_index,
        nchar_format = nchar_format::less);

    static std::string type(pageFileID const * pages, size_t page_size);

    template<size_t page_size>
    static std::string type(pageFileID const(&pages)[page_size]) {
        return type(pages, page_size);
    }
};

template <class T>
std::string to_string::type(T const & value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

struct page_info: is_static {
    static std::string type_meta(page_head const &);
    static std::string type_raw(page_head const &);
    static std::string type_meta(row_head const &);
};

namespace impl {

    template <typename T> struct identity
    {
        typedef T type;
    };

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

} // impl

struct to_string_with_head : to_string {
    using to_string::type; // allow type() methods from base class
    static std::string type(row_head const & h) {
        std::stringstream ss;
        ss << "\n";
        ss << page_info::type_meta(h);
        return ss.str();
    }
    template<class row_type, class col_type>
    static std::string type(row_type const * row, impl::identity<col_type>) {
        static_assert(std::is_same<typename col_type::type, nchar_range>::value, "nchar_range"); // supported type
        static_assert(null_bitmap_traits<row_type>::value, "null_bitmap");
        static_assert(variable_array_traits<row_type>::value, "variable_array");
        return to_string::type_nchar(
            row->data.head,
            col_type::offset,
            to_string::nchar_format::more);
    }
};

struct processor_row: is_static
{
    template<class row_type>
    static std::string type_meta(row_type const & data) {
        using type_list = typename row_type::meta::type_list;
        std::stringstream ss;
        impl::processor<type_list>::print(ss, &data, 
            impl::identity<to_string_with_head>());
        return ss.str();
    }
};

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_INFO_H__