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

    reference operator[](size_t const i) { 
        SDL_ASSERT((i < N) && "out of range"); 
        return elems[i];
    }        
    const_reference operator[](size_t const i) const {     
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
    static constexpr size_t BUF_SIZE = N; // static_size
    static constexpr size_t size() { return N; }
    static constexpr bool empty() { return false; }

    const T* data() const { return elems; }
    T* data() { return elems; }
};

template<class T, size_t N>
class vector_buf {
    using buf_type = array_t<T, N>;
    buf_type m_buf;
    std::vector<T> m_vec;
    size_t m_size;
    bool use_buf() const {
        return (m_size <= N);
    }
public:
    static constexpr size_t BUF_SIZE = N;
    typedef T              value_type;
    typedef T*             iterator;
    typedef const T*       const_iterator;
    typedef T&             reference;
    typedef const T&       const_reference;

    vector_buf(): m_size(0) {
        A_STATIC_ASSERT_TYPE(value_type, typename buf_type::value_type);
        A_STATIC_ASSERT_TYPE(iterator, typename buf_type::iterator);
        A_STATIC_ASSERT_TYPE(const_iterator, typename buf_type::const_iterator);
        A_STATIC_ASSERT_TYPE(const_reference, typename buf_type::const_reference);
#if SDL_DEBUG
        memset_zero(m_buf);
#endif
    }
    size_t empty() const {
        return 0 == m_size;
    }
    size_t size() const {
        return m_size;
    }
    size_t capacity() const {
        return use_buf() ? N : m_vec.capacity();
    }
    const_iterator data() const {
        return use_buf() ? m_buf.data() : m_vec.data();
    }
    iterator data() {
        return use_buf() ? m_buf.data() : m_vec.data();
    }
    const_iterator begin() const {
        return data();
    }
    const_iterator end() const {
        return data() + size(); 
    }
    iterator begin() {
        return data();
    }
    iterator end() {
        return data() + size(); 
    }
    const_reference operator[](size_t const i) const {
        SDL_ASSERT(i < m_size); 
        return use_buf() ? m_buf[i] : m_vec[i];
    }
    reference operator[](size_t const i) { 
        SDL_ASSERT(i < m_size); 
        return use_buf() ? m_buf[i] : m_vec[i];
    }        
    void push_back(const T &);

    template<class fun_type>
    void sort(fun_type comp) {
        std::sort(begin(), end(), comp);
    }
    void sort() {
        std::sort(begin(), end());
    }
    void clear() {
        if (!use_buf()) {
            SDL_ASSERT(!m_vec.empty());
            m_vec.clear();
        }
        m_size = 0;
    }
    void fill(const T & value) {
        A_STATIC_ASSERT_IS_POD(T);
        memset(begin(), value, sizeof(T) * size());
    }
private:
    void reserve(size_t const s) {
        if (s > N) {
            m_vec.reserve(s);
        }
    }
};

template<class T, size_t N>
void vector_buf<T, N>::push_back(const T & value) {
    if (m_size < N) {
        m_buf[m_size++] = value;
        SDL_ASSERT(use_buf());
    }
    else {
        if (N == m_size) {
            m_vec.reserve(N + 1);
            m_vec.assign(m_buf.begin(), m_buf.end());
        }
        m_vec.push_back(value);
        ++m_size;
        SDL_ASSERT(m_size == m_vec.size());
        SDL_ASSERT(capacity() >= m_size);
        SDL_ASSERT(!use_buf());
    }
}

} // namespace sdl

#endif // __SDL_COMMON_ARRAY_H__
