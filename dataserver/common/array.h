// array.h
//
#pragma once
#ifndef __SDL_COMMON_ARRAY_H__
#define __SDL_COMMON_ARRAY_H__

#include "dataserver/common/algorithm.h"

namespace sdl {

template<class T>
struct is_ctor_0 { // initially zero
    enum { value = false };
};

template<class T>
struct is_fill_0 { // can use memset and memcpy
    enum { value = std::is_pod<T>::value };
};

template<class T, size_t N>
struct array_t { // fixed-size array of elements of type T

    typedef T elems_t[N];
    elems_t elems;

    typedef T              value_type;
    typedef T*             iterator;
    typedef const T*       const_iterator;
    typedef T&             reference;
    typedef const T&       const_reference;

    static_assert(N, "not empty size");
    static constexpr size_t size() { return N; }
    static constexpr bool empty() { return false; }

    iterator        begin() noexcept      { return elems; }
    const_iterator  begin() const noexcept { return elems; }
    const_iterator cbegin() const noexcept { return elems; }
        
    iterator        end() noexcept       { return elems+N; }
    const_iterator  end() const noexcept { return elems+N; }
    const_iterator cend() const noexcept { return elems+N; }

    reference operator[](size_t const i) noexcept { 
        SDL_ASSERT((i < N) && "out of range"); 
        return elems[i];
    }        
    const_reference operator[](size_t const i) const noexcept {     
        SDL_ASSERT((i < N) && "out of range"); 
        return elems[i]; 
    }
    reference front() noexcept { 
        return elems[0]; 
    }        
    const_reference front() const noexcept {
        return elems[0];
    }        
    reference back() noexcept { 
        return elems[N-1]; 
    }        
    const_reference back() const noexcept { 
        return elems[N-1]; 
    }
    const T* data() const noexcept { return elems; }
    T* data() noexcept { return elems; }

    void fill_0() noexcept {
        static_assert(is_fill_0<T>::value, "is_fill_0");
        memset_zero(elems);
    }
    void fill_0(size_t const count) noexcept {
        static_assert(is_fill_0<T>::value, "is_fill_0");
        SDL_ASSERT(count <= N);
        memset(&elems, 0, sizeof(T) * count);
    }
    void copy_from(array_t const & src, size_t const count) noexcept {
        static_assert(std::is_nothrow_copy_assignable<value_type>::value, "");
        SDL_ASSERT(count <= N);
        std::copy(src.elems, src.elems + count, elems);
    }
};

} // namespace sdl

#endif // __SDL_COMMON_ARRAY_H__
