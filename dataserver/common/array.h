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

    static_assert(N, "not empty size");
    static constexpr size_t BUF_SIZE = N; // static_size
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
        A_STATIC_ASSERT_IS_POD(T);
        memset_zero(elems);
    }
    void copy_from(array_t const & src, size_t const count) noexcept {
        static_assert(std::is_nothrow_copy_assignable<value_type>::value, "");
        SDL_ASSERT(count <= N);
        std::copy(src.elems, src.elems + count, elems);
    }
};

template<class T>
class unique_vec : noncopyable {
public:
    using vector_type = std::vector<T>;
    unique_vec() = default;
    unique_vec(size_t const count, const T & value) {
        if (count) {
            m_p.reset(new vector_type(count, value));
        }
    }
    unique_vec(unique_vec && src) noexcept
        : m_p(std::move(src.m_p)) {}
    const unique_vec & operator=(unique_vec && src) {
        this->swap(src);
        return *this;
    }
    bool empty() const noexcept {
        return m_p ? m_p->empty() : true;
    }
    size_t size() const noexcept {
        return m_p ? m_p->size() : 0;
    }
    void clear() noexcept {
        if (m_p) m_p->clear();
    }
	explicit operator bool() const noexcept {
	    return !empty();
	}
    void swap(unique_vec & src) noexcept {
        m_p.swap(src.m_p);
    }
	vector_type * get() {
        if (!m_p) {
            m_p.reset(new vector_type);
        }
        return m_p.get();
    }
	vector_type const * get() const {
        if (!m_p) {
            m_p.reset(new vector_type);
        }
        return m_p.get();
    }
	vector_type * operator->() {
        return get();
    }
	vector_type const * operator->() const {
        return get();
    }
    unique_vec clone() const {
        unique_vec result;
        if (m_p) {
           result.m_p.reset(new vector_type(*m_p));
        }
        return result;
    }
private:
    mutable std::unique_ptr<vector_type> m_p;
};

template<class T, size_t N>
class vector_buf {
    using buf_type = array_t<T, N>;
    using vec_type = unique_vec<T>;
    size_t m_size; //note: could use pair m_begin, m_end to speed up operator[]
    vec_type m_vec;
    buf_type m_buf;
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

    vector_buf() noexcept : m_size(0) {
        debug_clear_pod(m_buf);
    }
    vector_buf(T const & init) noexcept : m_size(1) {
        debug_clear_pod(m_buf);
        m_buf[0] = init;
    }
    explicit vector_buf(size_t const count, const T & value = T())
        : m_size(count)
        , m_vec((count > N )? count : 0, value) {
        debug_clear_pod(m_buf);
        if (use_buf()) {
            fill(value);
        }
    }
    vector_buf(vector_buf && src) noexcept
        : m_size(src.m_size) {
        debug_clear_pod(m_buf);
        if (use_buf()) {
            m_buf.copy_from(src.m_buf, m_size);
        }
        else {
            m_vec.swap(src.m_vec);
            src.m_size = 0;
        }
    }
private:
    vector_buf(const vector_buf &);
    vector_buf& operator=(const vector_buf& src) = delete;
public:
    vector_buf clone() const { // can be slow
        return vector_buf(*this);
    }
    void swap(vector_buf &) noexcept;
    vector_buf & operator=(vector_buf && src) noexcept;
    bool use_buf() const noexcept {
        return (m_size <= N);
    }
    bool empty() const noexcept {
        return 0 == m_size;
    }
    size_t size() const noexcept {
        return m_size;
    }
    size_t capacity() const noexcept {
        return use_buf() ? N : m_vec->capacity();
    }
    const_iterator data() const noexcept {
        return use_buf() ? m_buf.data() : m_vec->data();
    }
    iterator data() noexcept {
        return use_buf() ? m_buf.data() : m_vec->data();
    }
    const_iterator begin() const noexcept {
        return data();
    }
    const_iterator end() const noexcept {
        return data() + size(); 
    }
    const_iterator cbegin() const noexcept {
        return begin();
    }
    const_iterator cend() const noexcept {
        return end(); 
    }
    iterator begin() noexcept {
        return data();
    }
    iterator end() noexcept {
        return data() + size(); 
    }
    const_reference front() const noexcept {
        SDL_ASSERT(!empty());
        return * begin();
    } 
    reference front() noexcept {
        SDL_ASSERT(!empty());
        return * begin();
    } 
    const_reference back() const noexcept {
        SDL_ASSERT(!empty());
        return *(end() - 1);
    }
    reference back() noexcept {
        SDL_ASSERT(!empty());
        return *(end() - 1);
    }
    const_reference operator[](size_t const i) const noexcept {
        SDL_ASSERT(i < m_size); 
        return begin()[i];
    }
    reference operator[](size_t const i) noexcept { 
        SDL_ASSERT(i < m_size); 
        return begin()[i];
    }        
    void push_back(const T &);
    template<typename... Ts>
    void emplace_back(Ts&&... params) {
        this->push_back(T{std::forward<Ts>(params)...});
    }
    void push_sorted(const T & value) {
        algo::insertion_sort(*this, value);
    }
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
    void fill_0() noexcept {
        A_STATIC_ASSERT_IS_POD(T);
        memset(begin(), 0, sizeof(T) * size());
    }
    void fill(const T & value) {
        for (T & p : *this) {
            p = value;
        }
    }
    void rotate(size_t first, size_t n_first);
    void append(vector_buf && src);
    void append(const_iterator, const_iterator);
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
};

template<class T, size_t N>
vector_buf<T, N>::vector_buf(const vector_buf & src)
    : m_size(src.m_size)
    , m_vec(src.m_vec.clone()) {
    debug_clear_pod(m_buf);
    if (use_buf()) {
        m_buf.copy_from(src.m_buf, m_size);
    }
}

template<class T, size_t N> vector_buf<T, N> &
vector_buf<T, N>::operator=(vector_buf && src) noexcept {
    debug_clear_pod(m_buf);
    if (src.use_buf()) {
        if (!use_buf()) {
            SDL_ASSERT(!m_vec.empty());
            m_vec.clear();
        }
        m_buf.copy_from(src.m_buf, m_size = src.m_size);
    }
    else {
        SDL_ASSERT(src.m_vec.size() == src.m_size);
        m_vec.swap(src.m_vec);
        std::swap(m_size, src.m_size);
    }
    SDL_ASSERT(m_vec.size() == (use_buf() ? 0 : size()));
    return *this;
}

template<class T, size_t N>
void vector_buf<T, N>::push_back(const T & value) {
    if (m_size < N) {
        m_buf[m_size++] = value;
        SDL_ASSERT(use_buf());
    }
    else {
        auto vec = m_vec.get();
        if (N == m_size) { // m_buf is full
            vec->assign(m_buf.begin(), m_buf.end());
        }
        vec->push_back(value);
        ++m_size;
        SDL_ASSERT(m_size == vec->size());
        SDL_ASSERT(capacity() >= m_size);
        SDL_ASSERT(!use_buf());
    }
}

template<class T, size_t N>
void vector_buf<T, N>::swap(vector_buf & src) noexcept {
    bool const b1 = use_buf();
    bool const b2 = src.use_buf();
    if (b1 && b2) {
        T * p1 = m_buf.begin();
        T * p2 = src.m_buf.begin();
        size_t len = a_max(m_size, src.m_size);
        while (len--) {
            std::swap(*p1++, *p2++);
        }
    }
    else {
        if (b1) {
            SDL_ASSERT(!b2);
            debug_clear_pod(src.m_buf);
            src.m_buf.copy_from(m_buf, m_size);
        }
        else if (b2) {
            SDL_ASSERT(!b1);
            debug_clear_pod(m_buf);
            m_buf.copy_from(src.m_buf, src.m_size);
        }
        m_vec.swap(src.m_vec);
    }
    std::swap(m_size, src.m_size);
}

template<class T, size_t N>
void vector_buf<T, N>::append(vector_buf && src)
{
    const size_t new_size = m_size + src.m_size;
    if (new_size <= N) {
        T * p1 = m_buf.begin() + m_size;
        T * p2 = src.m_buf.begin();
        size_t len = src.m_size;
        while (len--) {
            *p1++ = std::move(*p2++);
        }
        m_size = new_size;
        SDL_ASSERT(use_buf());
    }
    else {
        if (!m_size) {
            m_vec.swap(src.m_vec);
            std::swap(m_size, src.m_size);
        }
        else {
            if (use_buf() || src.use_buf()) {
                for (auto const & p : src) {
                    push_back(p);
                }
            }
            else {
                SDL_ASSERT(!m_vec.empty());
                SDL_ASSERT(!src.m_vec.empty());
                m_vec->insert(m_vec->end(), src.begin(), src.end());
                SDL_ASSERT(m_vec->size() == new_size);
                m_size = new_size;
            }
        }
        SDL_ASSERT(m_size == new_size);
    }
}

template<class T, size_t N> inline
void vector_buf<T, N>::append(const_iterator first, const_iterator last) {
    SDL_ASSERT(first <= last);
    for (; first != last; ++first) {
        push_back(*first);
    }
}

template<class T, size_t N> inline
void vector_buf<T, N>::rotate(size_t const first, size_t const n_first) {
    SDL_ASSERT(first < n_first);
    SDL_ASSERT(n_first < size());
    auto const p = begin();
    std::rotate(p + first, p + n_first, p + size());
}

} // namespace sdl

#endif // __SDL_COMMON_ARRAY_H__
