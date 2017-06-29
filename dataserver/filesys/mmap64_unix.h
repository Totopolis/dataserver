// mmap64_unix.h
//
#pragma once
#ifndef __SDL_FILESYS_MMAP64_UNIX_H__
#define __SDL_FILESYS_MMAP64_UNIX_H__

#include "dataserver/common/common.h"

#if defined(SDL_OS_UNIX)

#include <sys/mman.h>

namespace sdl { namespace mmap64_detail {

struct substitution_failure {}; // represent a failure to declare something

template<typename T> 
struct substitution_succeeded : std::true_type {};
        
template<>
struct substitution_succeeded<substitution_failure> : std::false_type {};

struct get_mmap64 {
private:
    template<typename X>
    static auto check(X const& x) -> decltype(::mmap64(x, 0, 0, 0, 0, 0));
    static substitution_failure check(...);
public:
    using type = decltype(check(nullptr));
};

template<bool>
struct select_mmap64 {
    template<typename... Values>
    static void * get(Values&&... params) {
        return ::mmap(std::forward<Values>(params)...);
    }
};

template<>
struct select_mmap64<true> {
    template<typename... Values>
    static void * get(Values&&... params) {
        return ::mmap64(std::forward<Values>(params)...);
    }
};

struct has_mmap64: substitution_succeeded<get_mmap64::type> {};

} // mmap64_detail

using mmap64_t = mmap64_detail::select_mmap64<mmap64_detail::has_mmap64::value>;

} // sdl

#endif //#if defined(SDL_OS_UNIX)
#endif // __SDL_FILESYS_MMAP64_UNIX_H__