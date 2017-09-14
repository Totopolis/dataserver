// thread.h
#pragma once
#ifndef __SDL_COMMON_THREAD_H__
#define __SDL_COMMON_THREAD_H__

#include "dataserver/common/spinlock.h"
#include <thread>
#include <future>

namespace sdl {

template<class T> 
class scoped_shutdown : noncopyable {
    T & m_obj;
    std::future<void> m_fut; // wait for thread to join in std::future desructors
public:
    scoped_shutdown(T & obj, std::future<void> && f)
        : m_obj(obj), m_fut(std::move(f))
    {}
    ~scoped_shutdown() {
        m_obj.shutdown();
    }
    void get() { // can be used to process std::future exception
        m_fut.get();
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

using unique_thread = std::unique_ptr<joinable_thread>;

inline void sleep_for_milliseconds(size_t v) {
    std::this_thread::sleep_for(std::chrono::milliseconds(v));
}

} // sdl

#endif // __SDL_COMMON_THREAD_H__