// array.h
//
#pragma once
#ifndef __SDL_COMMON_ARRAY_H__
#define __SDL_COMMON_ARRAY_H__

namespace sdl {

template<class T, size_t N>
struct array_t { // fixed-size array of elements of type T

    typedef T elems_t[N];
    elems_t elems;

    typedef T              value_type;
    typedef T*             iterator;
    typedef const T*       const_iterator;
    typedef T&             reference;
    typedef const T&       const_reference;

    iterator        begin()       { return elems; }
    const_iterator  begin() const { return elems; }
    const_iterator cbegin() const { return elems; }
        
    iterator        end()       { return elems+N; }
    const_iterator  end() const { return elems+N; }
    const_iterator cend() const { return elems+N; }

    reference operator[](size_t i) { 
        SDL_ASSERT((i < N) && "out of range"); 
        return elems[i];
    }        
    const_reference operator[](size_t i) const {     
        SDL_ASSERT((i < N) && "out of range"); 
        return elems[i]; 
    }
    reference front() { 
        return elems[0]; 
    }        
    const_reference front() const {
        return elems[0];
    }        
    reference back() { 
        return elems[N-1]; 
    }        
    const_reference back() const { 
        return elems[N-1]; 
    }
    static const size_t static_size = N;
    static size_t size() { return N; }
    static bool empty() { return false; }

    const T* data() const { return elems; }
    T* data() { return elems; }
};

} // namespace sdl

#endif // __SDL_COMMON_ARRAY_H__
