// static_vector.h
//
#pragma once
#ifndef __SDL_COMMON_STATIC_VECTOR_H__
#define __SDL_COMMON_STATIC_VECTOR_H__

#include "dataserver/common/common.h"

namespace sdl {

template<class T, size_t N>
class static_vector {
    static_assert(N, "not empty size");
    typedef T elems_t[N];
    elems_t elems;
    size_t m_size = 0;
public:
    typedef T              value_type;
    typedef T*             iterator;
    typedef const T*       const_iterator;
    typedef T&             reference;
    typedef const T&       const_reference;

    static constexpr size_t capacity() { return N; }    
    const size_t size() const noexcept {
        return m_size;
    }
    bool empty() const noexcept {
        return 0 == size();
    }
    iterator        begin() noexcept       { return elems; }
    const_iterator  begin() const noexcept { return elems; }
    const_iterator cbegin() const noexcept { return elems; }
        
    iterator        end() noexcept       { return elems + size(); }
    const_iterator  end() const noexcept { return elems + size(); }
    const_iterator cend() const noexcept { return elems + size(); }

    reference operator[](size_t const i) noexcept { 
        SDL_ASSERT((i < size()) && "out of range"); 
        return elems[i];
    }        
    const_reference operator[](size_t const i) const noexcept {     
        SDL_ASSERT((i < size()) && "out of range"); 
        return elems[i]; 
    }
    reference front() noexcept { 
        SDL_ASSERT(!empty());
        return elems[0]; 
    }        
    const_reference front() const noexcept {
        SDL_ASSERT(!empty());
        return elems[0];
    }        
    reference back() noexcept { 
        SDL_ASSERT(!empty());
        return elems[size()-1]; 
    }        
    const_reference back() const noexcept { 
        SDL_ASSERT(!empty());
        return elems[size()-1]; 
    }
    const T * data() const noexcept { return elems; }
    T * data() noexcept { return elems; }

    void push_back(value_type const & value) {
        SDL_ASSERT(size() < capacity());
        elems[m_size++] = value;
    }
    void insert(iterator pos, value_type const & value) {
        SDL_ASSERT(size() < capacity());
        SDL_ASSERT(begin() <= pos);
        SDL_ASSERT(pos <= end());
        for (auto it = end(); it > pos; --it) {
            *it = std::move(*(it - 1));
        }
        *pos = value;
        ++m_size;
    }
    void erase(iterator pos) {
        SDL_ASSERT(!empty());
        SDL_ASSERT(begin() <= pos);
        SDL_ASSERT(pos < end());
        const auto last = end() - 1;
        for (; pos < last; ++pos) {
            *pos = std::move(*(pos + 1));
        }
        SDL_DEBUG_CPP(back() = {});
        --m_size;
    }
private:
    static void reserve(size_t const s) {
        SDL_ASSERT(s <= capacity());
    }
};

} // sdl

#endif // __SDL_COMMON_STATIC_VECTOR_H__
