// finally.h
//
#pragma once
#ifndef __SDL_COMMON_FINALLY_H__
#define __SDL_COMMON_FINALLY_H__

namespace sdl {

// final_action allows you to ensure something gets run at the end of a scope
template <class F>
class final_action
{
public:
    explicit final_action(F f) noexcept : f_(std::move(f)) {}

    final_action(final_action&& other) noexcept : f_(std::move(other.f_)), invoke_(other.invoke_)
    {
        other.invoke_ = false;
    }

    final_action(const final_action&) = delete;
    final_action& operator=(const final_action&) = delete;
    final_action& operator=(final_action&&) = delete;

    ~final_action() noexcept
    {
        if (invoke_) f_();
    }

private:
    F f_;
    bool invoke_{true};
};

// finally() - convenience function to generate a final_action
template <class F>
final_action<F> finally(const F& f) noexcept
{
    return final_action<F>(f);
}

template <class F>
final_action<F> finally(F&& f) noexcept
{
    return final_action<F>(std::forward<F>(f));
}

} // sdl

#endif // __SDL_COMMON_FINALLY_H__
