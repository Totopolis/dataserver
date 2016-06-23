// noncopyable.h
//
#pragma once
#ifndef __SDL_COMMON_NONCOPYABLE_H__
#define __SDL_COMMON_NONCOPYABLE_H__

namespace sdl {

/*
Using ADL, unqualified function names are looked up in the namespaces
in which the arguments to the function are defined.

Presumably if noncopyable were defined in namespace <name>, then
function calls involving classes deriving from noncopyable would get
namespace <name> added to the ADL scopes in which the function is looked up.

Usually this isn't wanted, as noncopyable is an implementation detail,
not an interface.
*/

namespace noncopyable_  // protection from unintended ADL
{
    class is_static {
        is_static() = delete;
    };
#if 1
    class noncopyable {
    protected:
        noncopyable() = default;
        ~noncopyable() = default;

        noncopyable(const noncopyable&) = delete;
        noncopyable& operator=(const noncopyable&) = delete;
    };
#else // reserved
    template<bool> class noncopyable_t {
    protected:
        noncopyable_t() = default;
        ~noncopyable_t() = default;
    };
    template<> class noncopyable_t<true> {
    protected:
        noncopyable_t() = default;
        ~noncopyable_t() = default;

        noncopyable_t(const noncopyable_t&) = delete;
        noncopyable_t& operator=(const noncopyable_t&) = delete;
    };
    using noncopyable = noncopyable_t<true>;
#endif
}

typedef noncopyable_::noncopyable noncopyable;
typedef noncopyable_::is_static is_static;

} // namespace sdl

#endif // __SDL_COMMON_NONCOPYABLE_H__