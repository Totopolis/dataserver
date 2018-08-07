// union_find_t.h
//
#pragma once
#ifndef __SDL_NUMERIC_UNION_FIND_T_H__
#define __SDL_NUMERIC_UNION_FIND_T_H__

#include "dataserver/common/common.h"

namespace sdl {

/* WEIGHTED QUICK UNION WITH HALVING ALGORITHM */
template<typename T>
class union_find_t : noncopyable {
public:
    using value_type = T;
    explicit union_find_t(size_t);
    size_t capacity() const {
        return m_id.size();
    }
    size_t count() const { // Return the number of disjoint sets.
        SDL_ASSERT(m_count > 0);
        SDL_ASSERT(m_count <= capacity());
        return m_count;
    }
    size_t added() const { // number of registered components
        return m_added;
    }
    bool insert(value_type);
    bool exist(value_type) const;
    value_type find(value_type p); // Return component identifier for component containing p
    bool connected(value_type p, value_type q); // Are objects p and q in the same set?
    bool unite(value_type p, value_type q); // Replace sets containing p and q with their union.
    void clear();
private:
    using size_type = unsigned int;
    std::vector<value_type> m_id;   // id[i] = parent of i
    std::vector<size_type> m_sz;    // sz[i] = number of objects in subtree rooted at i
    size_t m_count = 0;             // number of components
    size_t m_added = 0;
};

template<typename T>
union_find_t<T>::union_find_t(size_t const N)
    : m_id(N)
    , m_sz(N, 0)
    , m_count(N)
{
    value_type i = 0;
    for (auto & id : m_id) {
        id = i++;
    }
}

template<typename T>
void union_find_t<T>::clear() {
    m_added = 0;
    m_count = m_id.size();
    m_sz.assign(m_sz.size(), 0);
    value_type i = 0;
    for (auto & id : m_id) {
        id = i++;
    }
}

template<typename T>
T union_find_t<T>::find(value_type p) {
    SDL_ASSERT(0 <= p);
    SDL_ASSERT(static_cast<size_t>(p) < m_id.size());
    for (;;) {
        value_type & q = m_id[p];
        if (p == q)
            return p;
        p = (q = m_id[q]); // compress path to tree root
    }
}

template<typename T>
inline bool union_find_t<T>::connected(value_type const p, value_type const q) {
    return find(p) == find(q);
}

template<typename T>
inline bool union_find_t<T>::exist(value_type const p) const {
    return m_sz[p] > 0;
}

template<typename T>
inline bool union_find_t<T>::insert(value_type const p) {
    if (!exist(p)) {
        m_sz[p] = 1;
        ++m_added;
        return true;
    }
    return false;
}

template<typename T>
bool union_find_t<T>::unite(value_type const p, value_type const q) {
    SDL_ASSERT(p != q);
    auto i = find(p);
    auto j = find(q);
    if (i == j) {
        SDL_ASSERT(m_count > 0);
        SDL_ASSERT(exist(p));
        SDL_ASSERT(exist(q));
        return false;
    }
    SDL_ASSERT(p != q);
    SDL_ASSERT(m_count > 1);
    if (!exist(p)) {
        SDL_ASSERT(i == p);
        m_sz[p] = 1;
        ++m_added;
    }
    if (!exist(q)) {
        SDL_ASSERT(j == q);
        m_sz[q] = 1;
        ++m_added;
    }
    SDL_ASSERT(exist(j));
    SDL_ASSERT(exist(i));
    if (m_sz[j] < m_sz[i]) {
        std::swap(i, j);
    }
    SDL_ASSERT(m_sz[i] <= m_sz[j]);
    m_id[i] = j; // make smaller root point to larger one
    m_sz[j] += m_sz[i];
    --m_count;
    return true;
} 

} // sdl

#endif // __SDL_NUMERIC_UNION_FIND_T_H__
