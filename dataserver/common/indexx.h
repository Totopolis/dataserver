// indexx.h
//
#pragma once
#ifndef __SDL_COMMON_INDEXX_H__
#define __SDL_COMMON_INDEXX_H__

#include "dataserver/common/common.h"

namespace sdl { namespace small {

template<typename T, size_t N>
struct indexx {
    typedef T               value_type;
    typedef const T*        const_iterator;
    typedef T&              reference;
    typedef const T&        const_reference;
    static constexpr size_t capacity() {
        return N;
    }
    indexx() = default;
    explicit indexx(size_t);

    bool empty() const noexcept {
        return !m_size;
    }
    size_t size() const noexcept {
        return m_size;
    }
    const_iterator begin() const noexcept { 
        return m_index;
    }
    const_iterator end() const noexcept {
        return m_index + m_size; 
    }
    T insert();
    T select(size_t);

    template<class fun_type>
    bool find(fun_type &&) const;

    using T_bool = std::pair<T, bool>;   
    
    template<class fun_type>
    T_bool find_and_select(fun_type &&);
private:
    T m_index[N] = {};
    size_t m_size = 0;
};

template<typename T, size_t N>
indexx<T, N>::indexx(const size_t sz): m_size(sz) {
    A_STATIC_CHECK_IS_POD(m_index);
    SDL_ASSERT(sz <= N);
    for (size_t i = 0; i < sz; ++i) {
        m_index[i] = static_cast<T>(i);
    }
}

template<typename T, size_t N>
T indexx<T, N>::insert() {
    if (m_size) {
        if (m_size == N) {
            const T i = m_index[N - 1];
            std::memmove(m_index + 1, m_index, (N - 1) * sizeof(T));
            m_index[0] = i;
            return i;
        }
        else {
            SDL_ASSERT(m_size < N);
            std::memmove(m_index + 1, m_index, (m_size - 1) * sizeof(T));
            return m_index[0] = static_cast<T>(m_size++);
        }
    }
    SDL_ASSERT(m_index[0] == 0);
    m_size = 1;
    return 0;
}

template<typename T, size_t N>
T indexx<T, N>::select(size_t const pos) {
    SDL_ASSERT(m_size <= N);
    SDL_ASSERT(pos < m_size);
    if (pos) {
        const T i = m_index[pos];
        std::memmove(m_index + 1, m_index, pos * sizeof(T));
        m_index[0] = i;
        return i;
    }
    return m_index[0];
}

template<typename T, size_t N>
template<class fun_type>
bool indexx<T, N>::find(fun_type && fun) const {
    for (const T i : m_index) {
        if (fun(i)) {
            return true;
        }
    }
    return false;
}

template<typename T, size_t N>
template<class fun_type> 
typename indexx<T, N>::T_bool
indexx<T, N>::find_and_select(fun_type && fun) {
    size_t pos = 0;
    for (const T i : m_index) {
        if (fun(i)) {
            return { select(pos), true };
        }
        ++pos;
    }
    return {};
}

using indexx_uint8 = indexx<uint8, 256>;

} // small
} // sdl
#endif // __SDL_COMMON_INDEXX_H__