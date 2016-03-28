// page_meta.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_META_H__
#define __SDL_SYSTEM_PAGE_META_H__

#include <cstddef>

#include "common/type_seq.h"
#include "common/static_type.h"

namespace sdl { namespace db {

namespace meta {
    extern const char empty[];
    template<size_t _offset, class T, const char* _name, bool _var = false>
    struct col_type {
        enum { offset = _offset };
        enum { variable = _var };
        typedef T type;
        static const char * name() { return _name; }
    };
    template<size_t index, class T, const char* name>
    using var_col_type = col_type<index, T, name, true>;

} // meta

#define typedef_col_type(pagetype, member) \
    typedef meta::col_type<offsetof(pagetype, data.member), \
    decltype(pagetype().data.member), meta::empty> member

#define typedef_col_type_n(pagetype, member) \
    static const char c_##member[]; \
    typedef meta::col_type<offsetof(pagetype, data.member), \
    decltype(pagetype().data.member), c_##member> member

#define typedef_var_col(colindex, coltype, member) \
    static const char c_##member[]; \
    typedef meta::var_col_type<colindex, coltype, c_##member> member

#define static_col_name(pagetype, member) \
    const char pagetype::c_##member[] = #member

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_META_H__
