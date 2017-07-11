// spinlock.h
#pragma once
#ifndef __SDL_COMMON_SPINLOCK_H__
#define __SDL_COMMON_SPINLOCK_H__

#include "dataserver/common/common.h"
#include <atomic>
#include <thread>

namespace sdl {

struct atomic_flag {
    std::atomic_flag value { ATOMIC_FLAG_INIT };
};

class spin_lock : noncopyable {
    std::atomic_flag & m_flag;
public:
    explicit spin_lock(std::atomic_flag & f): m_flag(f) {
        while (m_flag.test_and_set(std::memory_order_acquire)) { // acquire lock
            std::this_thread::yield(); // spin
        }
    }
    ~spin_lock(){
        m_flag.clear(std::memory_order_release); // release lock
    }
};

class joinable_thread : noncopyable {
    std::thread m_thread;
public:
    template<typename... Ts>
    explicit joinable_thread(Ts&&... params)
        : m_thread(std::forward<Ts>(params)...)
    {}
    ~joinable_thread() {
        m_thread.join();
    }
};

class joinable_thread_ex : noncopyable {
    std::function<void()> m_exit;
    std::thread m_thread;
public:
    template<class on_exit, typename... Ts>
    joinable_thread_ex(on_exit fun, Ts&&... params)
        : m_exit(fun), m_thread(std::forward<Ts>(params)...)
    {}
    ~joinable_thread_ex() {
        m_exit();
        m_thread.join();
    }
};

inline void sleep_for_milliseconds(size_t v) {
    std::this_thread::sleep_for(std::chrono::milliseconds(v));
}

} // sdl

#endif // __SDL_COMMON_SPINLOCK_H__