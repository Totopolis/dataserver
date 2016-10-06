// index_page.h
//
#pragma once
#ifndef __SDL_SYSTEM_INDEX_PAGE_H__
#define __SDL_SYSTEM_INDEX_PAGE_H__

#include "page_head.h"
#include "scalartype_t.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

template<class T>
struct index_page_row_t // > 7 bytes
{
    using key_type = T;

    struct data_type {
        bitmask8    statusA;    // 1 byte
        key_type    key;        // primary key
        pageFileID  page;       // 6 bytes
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    recordType get_type() const { // Bits 1-3 of byte 0 give the record type
        return static_cast<recordType>((data.statusA.byte & 0xE) >> 1);
    }
};

enum { index_row_head_size = sizeof(bitmask8) + sizeof(pageFileID) };

#pragma pack(pop)

namespace impl {

template<scalartype::type v, typename T>
struct index_key_t {
    static const scalartype::type value = v;
    using type = T;
};

} // impl

template<scalartype::type v> using index_key_t = impl::index_key_t<v, scalartype_t<v>>;
template<scalartype::type v> using index_key = typename index_key_t<v>::type;

template<class T>
std::ostream & operator <<(std::ostream & out, pair_key<T> const & i) {
    out << "(" << i.first << ", " << i.second << ")";
    return out;
}

template<scalartype::type v> inline 
scalartype_t<v> const * index_key_cast(mem_range_t const & m) {
    using T = scalartype_t<v>;
    if (mem_size(m) == sizeof(T)) {
        return reinterpret_cast<T const *>(m.first);
    }
    SDL_ASSERT(0);
    return nullptr; 
}

template<class fun_type> 
struct case_index_ret_type {
private:
    template<typename T> static auto check(void *) -> typename T::ret_type;
    template<typename T> static void check(...);
public:
    using type = decltype(check<fun_type>(nullptr));
};

template<class fun_type>
typename case_index_ret_type<fun_type>::type
case_index_key(scalartype::type const v, fun_type && fun) {
    switch (v) {
    case scalartype::t_int              : return fun(index_key_t<scalartype::t_int>());
    case scalartype::t_bigint           : return fun(index_key_t<scalartype::t_bigint>());
    case scalartype::t_uniqueidentifier : return fun(index_key_t<scalartype::t_uniqueidentifier>());
    case scalartype::t_float            : return fun(index_key_t<scalartype::t_float>());
    case scalartype::t_real             : return fun(index_key_t<scalartype::t_real>());
    default:
        return fun.unexpected(v);
    }
}

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_PAGE_H__