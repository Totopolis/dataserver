// mem_utils.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEM_UTILS_H__
#define __SDL_SYSTEM_MEM_UTILS_H__

#include "common/static.h"
#include "common/static_type.h"
#include "common/array.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct nchar_t // 2 bytes
{
    uint16 _16;
};

inline nchar_t nchar(uint16 i) {
    return { i };
}

template<class T>
struct base_mem_range
{
    using first_type = T;
    using second_type = T;

    first_type first;
    second_type second;

    base_mem_range() noexcept : first(nullptr), second(nullptr) {}
    base_mem_range(first_type p1, second_type p2) noexcept
        : first(p1), second(p2) {
        SDL_ASSERT(first <= second);
        SDL_ASSERT((first == nullptr) == (second == nullptr));
        static_assert(sizeof(T) == sizeof(void*), "");
    }
};

#pragma pack(pop)

inline bool operator == (nchar_t x, nchar_t y) { return x._16 == y._16; }
inline bool operator != (nchar_t x, nchar_t y) { return x._16 != y._16; }

inline bool operator < (nchar_t x, nchar_t y) {
    return x._16 < y._16;
}

using nchar_range = base_mem_range<const nchar_t *>;

nchar_t const * reverse_find(
    nchar_t const * const begin,
    nchar_t const * const end,
    nchar_t const * const buf,
    size_t const buf_size);

template<size_t buf_size> inline
nchar_t const * reverse_find(nchar_range const & s, nchar_t const(&buf)[buf_size]) {
    return reverse_find(s.first, s.second, buf, buf_size);
}

using mem_range_t = base_mem_range<const char *>;

inline bool operator == (mem_range_t const & x, mem_range_t const & y) {
    return (x.first == y.first) && (x.second == y.second);
}
inline bool operator != (mem_range_t const & x, mem_range_t const & y) {
    return !(x == y);
}

using vector_mem_range_t = vector_buf<mem_range_t, 2>;

inline void append(vector_mem_range_t & dest, vector_mem_range_t && src) {
    dest.append(std::move(src));
}
inline void append(vector_mem_range_t & dest, mem_range_t const * begin, mem_range_t const * end) {
    dest.append(begin, end);
}

inline size_t mem_size(mem_range_t const & m) {
    SDL_ASSERT(m.first <= m.second);
    return (m.second - m.first);
}

inline bool mem_empty(mem_range_t const & m) {
    return 0 == mem_size(m);
}

inline int mem_compare(mem_range_t const & x, mem_range_t const & y) {
    if (const int i = static_cast<int>(mem_size(x) - mem_size(y))) {
        return i;
    }
    return ::memcmp(x.first, y.first, mem_size(x)); 
}

inline bool mem_empty(vector_mem_range_t const & array) {
    for (auto const & m : array) {
        if (!mem_empty(m)) return false;
        SDL_ASSERT(0); // warning
    }
    return true;
}

inline size_t mem_size_n(vector_mem_range_t const & data) {
    size_t size = 0;
    for (auto const & m : data) {
        SDL_ASSERT(!mem_empty(m)); // warning
        size += mem_size(m);
    }
    return size;
}

inline size_t mem_size(vector_mem_range_t const & m) {
    return mem_size_n(m);
}

inline mem_range_t make_mem_range(std::vector<char> & buf) { // lvalue to avoid expiring buf
    auto const p = buf.data();
    return { p, p + buf.size() };
}

struct mem_utils : is_static {
    using vector_char = std::vector<char>;
    static vector_char make_vector(vector_mem_range_t const &); // note performance!
    static vector_char make_vector_n(vector_mem_range_t const &, size_t); // note performance!
    static bool memcpy_n(void * dest, vector_mem_range_t const &, size_t);

    template<class T>
    static bool memcpy_n(T & dest, vector_mem_range_t const & src) {
        A_STATIC_ASSERT_IS_POD(T);
        return memcpy_n(&dest, src, sizeof(dest));
    }
};

inline nchar_range make_nchar(mem_range_t const & m) {
    SDL_ASSERT(m.first <= m.second);
    SDL_ASSERT(!(mem_size(m) % sizeof(nchar_t)));
    return {
        reinterpret_cast<nchar_t const *>(m.first),
        reinterpret_cast<nchar_t const *>(m.second) };
}

inline nchar_range make_nchar_checked(mem_range_t const & m) {
    if (!(mem_size(m) % sizeof(nchar_t))) {
        return {
            reinterpret_cast<nchar_t const *>(m.first),
            reinterpret_cast<nchar_t const *>(m.second) };
    }
    SDL_ASSERT(0);
    return {};
}

inline bool nchar_less(nchar_t const x[], nchar_t const y[], size_t const N) {
    return std::lexicographical_compare(x, x + N, y, y + N);
}

template<size_t N>
inline bool nchar_less(nchar_t const (&x)[N], nchar_t const (&y)[N]) {
    return nchar_less(x, y, N);
}

inline int nchar_compare(nchar_t const x[], nchar_t const y[], size_t const N) {
    if (nchar_less(x, y, N)) return -1;
    if (nchar_less(y, x, N)) return 1;
    return 0;
}

template<size_t N>
inline int nchar_compare(nchar_t const (&x)[N], nchar_t const (&y)[N]) {
    return nchar_compare(x, y, N);
}

template<class T>
class mem_array_t {
public:
    const mem_range_t data;
    mem_array_t() {} //= default;
    explicit mem_array_t(mem_range_t const & d) noexcept : data(d) {
        SDL_ASSERT(!((data.second - data.first) % sizeof(T)));
        SDL_ASSERT((end() - begin()) == size());
        static_assert_is_nothrow_move_assignable(mem_range_t);
    }
    mem_array_t(const T * b, const T * e) noexcept
        : data((const char *)b, (const char *)e)
    {
        SDL_ASSERT(b <= e);
    }
    bool empty() const noexcept {
        return (data.first == data.second); // return 0 == size();
    }
    size_t size() const noexcept {
        return (data.second - data.first) / sizeof(T);
    }
    T const & operator[](size_t i) const noexcept {
        SDL_ASSERT(i < size());
        return *(begin() + i);
    }
    T const * begin() const noexcept {
        return reinterpret_cast<T const *>(data.first);
    }
    T const * end() const noexcept {
        return reinterpret_cast<T const *>(data.second);
    }
};

class var_mem { // movable
    using data_type = vector_mem_range_t;
    data_type m_data;
    var_mem(const var_mem&) = delete;
    const var_mem& operator=(const var_mem&) = delete;
public:
    using iterator = data_type::const_iterator;
    var_mem() = default;
    ~var_mem() = default;
    var_mem(data_type && v) noexcept 
        : m_data(std::move(v)) {
        static_assert_is_nothrow_move_assignable(data_type);
    }
    var_mem(var_mem && v) noexcept 
        : m_data(std::move(v.m_data)) {}
    var_mem & operator=(var_mem && v) noexcept {
        m_data.swap(v.m_data);
        return *this;
    }
    iterator begin() const noexcept { return m_data.cbegin(); }
    iterator end() const noexcept { return m_data.cend(); }
    data_type const & data() const && noexcept = delete;
    data_type const & data() const & noexcept {
        return m_data;
    }
    data_type const & cdata() const && noexcept = delete;
    data_type const & cdata() const & noexcept {
        return m_data;
    }
    data_type release() noexcept {
        return std::move(m_data);
    }
};

} // db
} // sdl

#endif // __SDL_SYSTEM_MEM_UTILS_H__