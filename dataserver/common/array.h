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

    static constexpr size_t BUF_SIZE = N; // static_size
    static constexpr size_t size() { return N; }
    static constexpr bool empty() { return false; }

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
    const T* data() const { return elems; }
    T* data() { return elems; }

    void fill_0() {
        A_STATIC_ASSERT_IS_POD(T);
        memset_zero(elems);
    }
};


template<class T, size_t N>
class vector_buf {
    using buf_type = array_t<T, N>;
    buf_type m_buf;
    std::vector<T> m_vec;
    size_t m_size;
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
        static_assert(sizeof(m_buf) <= 1024, "limit stack usage");
#if SDL_DEBUG
        clear_if_pod(m_buf);
#endif
    }
    bool use_buf() const {
        return (m_size <= N);
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
    const_iterator cbegin() const {
        return begin();
    }
    const_iterator cend() const {
        return end(); 
    }
    iterator begin() {
        return data();
    }
    iterator end() {
        return data() + size(); 
    }
    const_reference front() const {
        SDL_ASSERT(!empty());
        return * begin();
    } 
    reference front() {
        SDL_ASSERT(!empty());
        return * begin();
    } 
    const_reference back() const {
        SDL_ASSERT(!empty());
        return *(end() - 1);
    }
    reference back() {
        SDL_ASSERT(!empty());
        return *(end() - 1);
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
    template<typename... Ts>
    void emplace_back(Ts&&... params) {
        this->push_back(T{std::forward<Ts>(params)...});
    }
    void push_unique_sorted(const T &);
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
#if SDL_DEBUG
        clear_if_pod(m_buf);
#endif
    }
    void fill_0() {
        A_STATIC_ASSERT_IS_POD(T);
        memset(begin(), 0, sizeof(T) * size());
    }
private:
    void reserve(size_t const s) {
        if (s > N) {
            m_vec.reserve(s);
        }
    }
private:
#if SDL_DEBUG
    static void clear_if_pod(buf_type & buf, std::false_type) {}
    static void clear_if_pod(buf_type & buf, std::true_type) {
        buf.fill_0();
    }
    static void clear_if_pod(buf_type & buf) {
        clear_if_pod(buf, bool_constant<std::is_pod<buf_type>::value>{});
    }
#endif
};

template<class T, size_t N>
void vector_buf<T, N>::push_back(const T & value) {
    if (m_size < N) {
        m_buf[m_size++] = value;
        SDL_ASSERT(use_buf());
    }
    else {
        if (N == m_size) {
            //SDL_WARNING_DEBUG_2(!"vector_buf");
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

template<class T, size_t N>
void vector_buf<T, N>::push_unique_sorted(const T & value) {
    A_STATIC_ASSERT_IS_POD(T);
    if (empty()) {
        push_back(value);
    }
    else { // https://en.wikipedia.org/wiki/Insertion_sort
        const iterator first = begin();
        iterator last = end() - 1;
        while (first <= last) {
            if (value == *last)
                return;
            if (value < *last) {
                --last;
            }
            else
                break;
        }
        if (first <= last) { //FIXME: insert after last 
            SDL_ASSERT(*last < value);
            size_t const pos = last - first + 1;
            if (pos < size()) {
                push_back(T()); // reserve place 
                iterator const place = begin() + pos;
                std::memmove(place + 1, place, sizeof(T) * (size() - pos));
                *place = value;
            }
            else {
                SDL_ASSERT(pos == size());
                SDL_ASSERT(last + 1 == end());
                push_back(value);
            }
        }
        else {
            SDL_ASSERT(last + 1 == first);
            SDL_ASSERT(value < *first);
            push_back(T()); // reserve place            
            std::memmove(first + 1, first, sizeof(T) * (size() - 1));
            front() = value;
        }
        SDL_ASSERT(std::is_sorted(cbegin(), cend()));
    }
}


} // namespace sdl

#endif // __SDL_COMMON_ARRAY_H__
