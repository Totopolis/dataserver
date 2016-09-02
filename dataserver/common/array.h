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
    size_t m_size = 0;
public:
    static constexpr size_t BUF_SIZE = N;
    typedef T              value_type;
    typedef T*             iterator;
    typedef const T*       const_iterator;
    typedef T&             reference;
    typedef const T&       const_reference;

    A_STATIC_ASSERT_TYPE(value_type, typename buf_type::value_type);
    A_STATIC_ASSERT_TYPE(iterator, typename buf_type::iterator);
    A_STATIC_ASSERT_TYPE(const_iterator, typename buf_type::const_iterator);
    A_STATIC_ASSERT_TYPE(const_reference, typename buf_type::const_reference);
    static_assert(sizeof(buf_type) <= 1024, "limit stack usage");

    vector_buf() {
        debug_clear_pod(m_buf);
    }
    explicit vector_buf(size_t const count, const T & value = T())
        : m_size(count)
        , m_vec((count > N )? count : 0, value) {
        debug_clear_pod(m_buf);
        if (use_buf()) {
            fill(value);
        }
    }
    vector_buf(vector_buf && src): m_size(src.m_size) {
        if (use_buf()) {
            move_buf(src.m_buf.begin());
        }
        else {
            m_vec.swap(src.m_vec);
            src.m_size = 0;
        }
    }
    const vector_buf & operator=(vector_buf && src) {
        if (src.use_buf()) {
            if (!use_buf()) {
                SDL_ASSERT(!m_vec.empty());
                m_vec.clear();
            }
            m_size = src.m_size;
            move_buf(src.m_buf.begin());
        }
        else {
            std::swap(m_size, src.m_size);
            m_vec.swap(src.m_vec);
        }
        SDL_ASSERT(m_vec.size() == (use_buf() ? 0 : size()));
        return *this;
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
        return begin()[i];
    }
    reference operator[](size_t const i) { 
        SDL_ASSERT(i < m_size); 
        return begin()[i];
    }        
    void push_back(const T &);
    template<typename... Ts>
    void emplace_back(Ts&&... params) {
        this->push_back(T{std::forward<Ts>(params)...});
    }
    void push_sorted(const T &);
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
        debug_clear_pod(m_buf);
    }
    void fill_0() {
        A_STATIC_ASSERT_IS_POD(T);
        memset(begin(), 0, sizeof(T) * size());
    }
    void fill(const T & value) {
        for (T & p : *this) {
            p = value;
        }
    }
private:
    void reserve(size_t const s) {
        if (s > N) {
            m_vec.reserve(s);
        }
    }
private:
#if SDL_DEBUG
    static void debug_clear_pod(buf_type & buf, std::false_type) {}
    static void debug_clear_pod(buf_type & buf, std::true_type) {
        buf.fill_0();
    }
    static void debug_clear_pod(buf_type & buf) {
        debug_clear_pod(buf, bool_constant<std::is_pod<buf_type>::value>{});
    }
#else
    static void debug_clear_pod(buf_type &) {}
#endif
    void move_buf(T * src) {
        SDL_ASSERT(use_buf());
        T * p = m_buf.begin();
        T * const last = p + m_size;
        while (p != last) {
            *p++ = std::move(*src++);
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
void vector_buf<T, N>::push_sorted(const T & value) {
    push_back(value);
    iterator const left = begin();
    iterator right = end() - 1;
    while (right > left) {
        if (*right < *(right - 1)) {
            std::swap(*right, *(right - 1));
            --right;
        }
        else {
            break;
        }
    }
    SDL_ASSERT(left <= right);
    SDL_ASSERT(std::is_sorted(cbegin(), cend()));
}

} // namespace sdl

#endif // __SDL_COMMON_ARRAY_H__
