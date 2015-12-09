// page_meta.h
//
#ifndef __SDL_SYSTEM_PAGE_META_H__
#define __SDL_SYSTEM_PAGE_META_H__

#pragma once

#include "common/type_seq.h"
#include "common/static_type.h"

namespace sdl { namespace db {

namespace meta {
    extern const char empty[];
    template<size_t _offset, class T, const char* _name>
    struct col_type {
        enum { offset = _offset };
        typedef T type;
        static const char * name() { return _name; }
    };
} // meta

#define typedef_col_type(pagetype, member) \
    typedef meta::col_type<offsetof(pagetype, data.member), decltype(pagetype().data.member), meta::empty> member

#define typedef_col_type_n(pagetype, member) \
    static const char c_##member[]; \
    typedef meta::col_type<offsetof(pagetype, data.member), decltype(pagetype().data.member), c_##member> member

#define static_col_name(pagetype, member) \
    const char pagetype::c_##member[] = #member

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_META_H__