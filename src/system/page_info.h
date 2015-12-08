// page_info.h
//
#ifndef __SDL_SYSTEM_PAGE_INFO_H__
#define __SDL_SYSTEM_PAGE_INFO_H__

#pragma once

#include "page_head.h"
#include <sstream>

namespace sdl { namespace db {

template <unsigned long N>
struct binary;

template <unsigned long N>
struct binary
{
    static unsigned const value = 
        (binary<N / 10>::value << 1)    // prepend higher bits
            | (N % 10);                 // to lowest bit
};

template <> struct binary<0>
{
    static unsigned const value = 0;
};

struct to_string {

    static const char * type_name(pageType); 

    static std::string type(pageType);
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
    static std::string type(bitmask const &);

    template<size_t buf_size>
    static std::string type(nchar_t const(&buf)[buf_size]) {
        return type(buf, buf_size);
    }

    static std::string type_raw(char const * buf, size_t buf_size);

    template<size_t buf_size>
    static std::string type_raw(char const(&buf)[buf_size]) {
        return type_raw(buf, buf_size);
    }

    static std::string type(std::pair<nchar_t const *, nchar_t const *> const &);

    template <class T>
    static std::string type(T const & value) {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }
    static std::string dump(void const * _buf, size_t const buf_size);
    to_string() = delete;
};

struct page_info {
    page_info() = delete;
    static std::string type_meta(page_head const &);
    static std::string type_raw(page_head const &);
    static std::string type_meta(record_head const &);
};

namespace impl {

    template <typename T> struct identity
    {
        typedef T type;
    };

    template <class type_list> struct processor;

    template <> struct processor<NullType>
    {
        template<class stream_type, class data_type, class format>
        static void print(stream_type &, data_type const * const, format){}
    };

    template <class T, class U> // T = meta::col_type
    struct processor< Typelist<T, U> >
    {
        // format parameter allows to extend or replace to_string behavior
        template<class stream_type, class data_type, class format = identity<to_string> >
        static void print(stream_type & ss, data_type const * const data, format f = format())
        {
            typedef typename T::type value_type;
            char const * p = reinterpret_cast<char const *>(data);
            p += T::offset;
            value_type const & value = *reinterpret_cast<value_type const *>(p);
            ss << "0x" << std::uppercase << std::hex << T::offset << ": " << std::dec;
            ss << T::name() << " = ";
            ss << format::type::type(value);
            ss << std::endl;
            processor<U>::print(ss, data, f);
        }
    };

} // impl

struct to_string_with_head : to_string {
    using to_string::type; // allow type() methods from base class
    static std::string type(record_head const & h) {
        std::stringstream ss;
        ss << "\n";
        ss << page_info::type_meta(h);
        return ss.str();
    }
};

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_INFO_H__