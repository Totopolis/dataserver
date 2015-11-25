// page_info.h
//
#ifndef __SDL_SYSTEM_PAGE_INFO_H__
#define __SDL_SYSTEM_PAGE_INFO_H__

#pragma once

#include "page_head.h"
#include <sstream>

namespace sdl { namespace db {

struct to_string {

    to_string() = delete;

#if 0
    template <class T>
    static std::string type(T value) {
        static_assert(sizeof(value) <= sizeof(double), "");
        std::stringstream ss;
        ss << value;
        return ss.str();
    }
#endif

    static const char * type(pageType);    
    static std::string type(guid_t const &);
    static std::string type(pageLSN const &);
    static std::string type(pageFileID const &);
    static std::string type(pageXdesID const &);
    static std::string type(wchar_t const * buf, size_t buf_size);

    template<size_t buf_size>
    static std::string type(wchar_t const(&buf)[buf_size]) {
        return type(buf, buf_size);
    }

    static std::string type_raw(char const * buf, size_t buf_size, bool show_address);

    template<size_t buf_size>
    static std::string type_raw(char const(&buf)[buf_size]) {
        return type_raw(buf, buf_size, true);
    }

private:
    typedef TL::Seq<
        pageType
        , guid_t
        , pageLSN
        , pageFileID
        , pageXdesID
    >::Type type_list;

    template<class stream_type, class value_type>
    static void type_impl(stream_type & ss, value_type const & value, std::true_type) {
        ss << to_string::type(value);
    }
    template<class stream_type, class value_type>
    static void type_impl(stream_type & ss, value_type const & value, std::false_type) {
        ss << value;
    }
public:
    template<class stream_type, class value_type>
    static void type(stream_type & ss, value_type const & value) {
        enum { is_found = TL::IndexOf<type_list, value_type>::value != -1 };
        type_impl(ss, value, std::integral_constant<bool, is_found>());
    }
    template<class stream_type>
    static void type(stream_type & ss, uint8 value) {
        ss << int(value);
    }
};

struct page_info {
    page_info() = delete;
    static std::string type(page_header const &);
    static std::string type_meta(page_header const &);
    static std::string type_raw(page_header const &);
};

namespace impl {

    template <class type_list> struct processor;

    template <> struct processor<NullType>
    {
        template<class stream_type, class data_type>
        static void print(stream_type &, data_type const * const){}
    };

    template <class T, class U> // T = meta::col_type
    struct processor< Typelist<T, U> >
    {
        template<class stream_type, class data_type>
        static void print(stream_type & ss, data_type const * const data)
        {
            typedef typename T::type value_type;
            char const * p = reinterpret_cast<char const *>(data);
            p += T::offset;
            value_type const & value = *reinterpret_cast<value_type const *>(p);
            ss << "0x" << std::uppercase << std::hex << T::offset << ": " << std::dec;
            to_string::type(ss, value);
            ss << std::endl;
            processor<U>::print(ss, data);
        }
    };

} // impl
} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_INFO_H__