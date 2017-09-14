// spinlock.h
#pragma once
#ifndef __SDL_COMMON_SPINLOCK_H__
#define __SDL_COMMON_SPINLOCK_H__

#include "dataserver/common/common.h"
#include <atomic>

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

    void yield() {
        std::this_thread::yield();
    }
    void lock() {
        while (!(old = counter.exchange(locked_0))) { // try lock
            SDL_ASSERT(!one_writer);
            yield();
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

    void yield() {
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
        for (;;) {
            if (try_lock()) {
                fun();
                if (commit())
                    return;
            }
            yield();
        }
    }
    template<class result_type, class fun_type>
    void work(result_type & result, fun_type && fun) {
        for (;;) {
            if (try_lock()) {
                fun(result);
                if (commit())
                    return;
            }
            yield();
        }
    }
};

using writelock = writelock_t<uint32, false>;
using readlock = readlock_t<uint32>;

} // test
} // sdl

#endif // __SDL_COMMON_SPINLOCK_H__