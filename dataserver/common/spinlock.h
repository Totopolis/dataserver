// spinlock.h
#pragma once
#ifndef __SDL_COMMON_SPINLOCK_H__
#define __SDL_COMMON_SPINLOCK_H__

#include "dataserver/common/common.h"
#include <atomic>
#include <thread>

namespace sdl {

struct atomic_flag_init {
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

namespace test {

template<typename T, bool one_writer>
class writelock_t: noncopyable { // spinlock
public:
    using value_type = T;
    using atomic_counter = std::atomic<value_type>;
    static const value_type locked_0 = 0;
private:
    atomic_counter volatile & counter;
    value_type old = 0;

    void yield(size_t) {
        std::this_thread::yield();
    }
    void lock() {
        size_t i = 0;
        while (!(old = counter.exchange(locked_0))) { // try lock
            SDL_ASSERT(!one_writer);
            yield(++i);
        }
        SDL_ASSERT(old);
    }
    void unlock() {
        if (!(++old)) ++old;
        counter = old; // atomic store
    }
public:
    explicit writelock_t(atomic_counter volatile & c)
        : counter(c)
    {
        lock();
    }
    ~writelock_t(){
        unlock();
    }
};

template<typename T>
class readlock_t: noncopyable {
public:
    using value_type = T;
    using atomic_counter = std::atomic<value_type>;
private:
    atomic_counter volatile & counter;
    value_type old = 0;

    void yield(size_t) {
        std::this_thread::yield();
    }
public:
    explicit readlock_t(atomic_counter volatile & c)
        : counter(c)
    {}
    bool try_lock() {
        return (old = counter) != 0;  // atomic load
    }
    bool commit() {
        return old && (old == counter); // atomic load
    }
    template<class fun_type>
    void work(fun_type && fun) {
        size_t i = 0;
        for (;;) {
            if (try_lock()) {
                fun();
                if (commit())
                    return;
            }
            yield(++i);
        }
    }
    template<class result_type, class fun_type>
    void work(result_type & result, fun_type && fun) {
        size_t i = 0;
        for (;;) {
            if (try_lock()) {
                fun(result);
                if (commit())
                    return;
            }
            yield(++i);
        }
    }
};

using writelock = writelock_t<uint32, false>;
using readlock = readlock_t<uint32>;

} // test

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