// noncopyable.h
//
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
