// interval_set.h
//
#pragma once
#ifndef __SDL_SPATIAL_INTERVAL_SET_H__
#define __SDL_SPATIAL_INTERVAL_SET_H__

#include "dataserver/common/common.h"
#include "dataserver/system/page_iterator.h"
#include <set>

namespace sdl { namespace db {

template<typename pk0_type>
struct interval_distance {
    int operator()(pk0_type const & x, pk0_type const & y) const {
        static_assert(std::numeric_limits<pk0_type>::is_integer, "interval_distance");
        SDL_ASSERT(x < y);
        return static_cast<int>(y - x);
    }
};

template<typename pk0_type>
class interval_set : noncopyable {
    struct value_t {
        pk0_type key;
        mutable bool is_interval;
        value_t(pk0_type const & v): key(v), is_interval(false){} // allow convertion
    };
    static bool is_interval(value_t const & x) {
        return x.is_interval;
    }
    static void set_interval(value_t const & x) {
        x.is_interval = true;
    }
    static bool is_less(value_t const & x, value_t const & y) {
        return x.key < y.key;
    }
    static bool is_same(value_t const & x, value_t const & y) {
        return x.key == y.key;
    }
    static int distance(pk0_type const & x, pk0_type const & y) {
        return interval_distance<pk0_type>()(x, y);
    }
    static bool is_next(value_t const & x, value_t const & y) {
        SDL_ASSERT(is_less(x, y));
        return distance(x.key, y.key) == 1;
    }
    struct key_equal {
        bool operator () (value_t const & x, value_t const & y) const {
            return is_same(x, y);
        }
    };
    struct key_compare {
        bool operator () (value_t const & x, value_t const & y) const {
            return is_less(x, y);
        }
    };
    using set_type = std::set<value_t, key_compare>;
    using set_iterator = typename set_type::iterator;
    using set_const_iterator = typename set_type::const_iterator;
    using iterator_state = std::pair<set_const_iterator, int>;
private:
    std::unique_ptr<set_type> m_set;
    size_t m_size = 0;
public:
    using iterator = forward_iterator<interval_set const, iterator_state>;
    using const_iterator = iterator;
    using value_type = pk0_type;
    interval_set(): m_set(new set_type){}
    interval_set(interval_set && src) noexcept
        : m_set(std::move(src.m_set))
        , m_size(src.m_size){
        static_check_is_nothrow_move_assignable(m_set);
        src.m_size = 0;
        SDL_ASSERT(m_set);
        SDL_ASSERT(!src.m_set);
    }
    void swap(interval_set & src) noexcept {
        m_set.swap(src.m_set);
        std::swap(m_size, src.m_size);
    }
    interval_set & operator=(interval_set && v) noexcept {
        this->swap(v);
        return *this;
    }
    bool empty() const {
        SDL_ASSERT((m_size > 0) == (m_set && !m_set->empty()));
        return 0 == m_size;
    }
    void clear() {
        m_set->clear();
        m_size = 0;
    }
    size_t contains() const { // test only
        return m_set->size();
    }
    size_t size() const {
        SDL_ASSERT(m_size == cell_count());
        return m_size;
    }
    bool insert(pk0_type const & cell) {
        if (insert_without_size(cell)) {
            ++m_size;
            SDL_ASSERT(m_size == cell_count());
            return true;
        }
        return false;
    }
    bool find(pk0_type const &) const;
    
    template<class fun_type>
    break_or_continue for_each(fun_type &&) const;

    template<class fun_type>
    break_or_continue for_each2(fun_type &&) const;

    iterator begin() const;
    iterator end() const;

    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
private:
    size_t cell_count() const;
    bool insert_without_size(pk0_type const &);
    
    template<class fun_type> static
    break_or_continue for_range(pk0_type, pk0_type, fun_type &&);

    bool end_interval(set_iterator const & it) const {
        if (it != m_set->begin()) {
            auto p = it;
            return is_interval(*(--p));
        }
        return false;
    }
    static void start_interval(set_iterator const & it) {
        set_interval(*it);
    }
    void insert_interval(set_iterator const & hint, value_t && cell) {
        SDL_ASSERT(m_set->find(cell) == m_set->end());
        set_interval(cell);
        m_set->insert(hint, std::move(cell));
    }
    set_iterator previous(set_iterator it) {
        SDL_ASSERT(it != m_set->begin());
        return --it;
    }
    using const_iterator_bc = std::pair<set_const_iterator, break_or_continue>;
    template<class fun_type>
    const_iterator_bc for_interval(set_const_iterator, fun_type &&) const;
    template<class fun_type>
    const_iterator_bc for_interval2(set_const_iterator, fun_type &&) const;
private:
    friend iterator;
    bool assert_iterator_state(iterator_state const &) const;
    pk0_type dereference(iterator_state const &) const;
    void load_next(iterator_state &) const;
};

} // db
} // sdl

#include "dataserver/spatial/interval_set.hpp"

#endif // __SDL_SPATIAL_INTERVAL_SET_H__