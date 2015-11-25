// noncopyable.h
//
#ifndef __SDL_COMMON_NONCOPYABLE_H__
#define __SDL_COMMON_NONCOPYABLE_H__

#pragma once

namespace sdl {

    namespace noncopyable_  // protection from unintended ADL
    {
        class noncopyable
        {
        protected:
            noncopyable() = default;
            ~noncopyable() = default;

            noncopyable(const noncopyable&) = delete;
            const noncopyable& operator=(const noncopyable&) = delete;
        };
    }

    typedef noncopyable_::noncopyable noncopyable;

} // namespace sdl

#endif // __SDL_COMMON_NONCOPYABLE_H__