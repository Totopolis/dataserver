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

    class noncopyable {
    protected:
        noncopyable() = default;
        ~noncopyable() = default;

        noncopyable(const noncopyable&) = delete;
        noncopyable& operator=(const noncopyable&) = delete;
    };
}

typedef noncopyable_::noncopyable noncopyable;
typedef noncopyable_::is_static is_static;

} // namespace sdl

#endif // __SDL_COMMON_NONCOPYABLE_H__