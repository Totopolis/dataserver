// interval_set.h
//
#pragma once
#ifndef __SDL_SPATIAL_INTERVAL_SET_H__
#define __SDL_SPATIAL_INTERVAL_SET_H__

#include "dataserver/common/common.h"
#include <set>

namespace sdl { namespace db {

template<typename pk0_type>
struct interval_distance {
    size_t operator()(pk0_type const & x, pk0_type const & y) const {
        static_assert(std::numeric_limits<pk0_type>::is_integer, "interval_distance");
        SDL_ASSERT(x < y);
        return static_cast<size_t>(y - x);
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
    static size_t distance(pk0_type const & x, pk0_type const & y) {
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
private:
    using set_type = std::set<value_t, key_compare>;
    using iterator = typename set_type::iterator;
    using const_iterator = typename set_type::const_iterator;
    std::unique_ptr<set_type> m_set;
private:
    bool end_interval(iterator const & it) const {
        if (it != m_set->begin()) {
            iterator p = it;
            return is_interval(*(--p));
        }
        return false;
    }
    static void start_interval(iterator const & it) {
        set_interval(*it);
    }
    void insert_interval(iterator const & hint, value_t && cell) {
        SDL_ASSERT(m_set->find(cell) == m_set->end());
        set_interval(cell);
        m_set->insert(hint, std::move(cell));
    }
    iterator previous(iterator it) {
        SDL_ASSERT(it != m_set->begin());
        return --it;
    }
    using const_iterator_bc = std::pair<const_iterator, break_or_continue>;
    template<class fun_type>
    const_iterator_bc for_interval(const_iterator, fun_type &&) const;
public:
    interval_set(): m_set(new set_type){}
    interval_set(interval_set && src): m_set(std::move(src.m_set)) {}
    void swap(interval_set & src) {
        m_set.swap(src.m_set);
    }
    interval_set & operator=(interval_set && v) {
        m_set.swap(v.m_set);
        return *this;
    }
    bool empty() const {
        return m_set->empty();
    }
    void clear() {
        m_set->clear();
    }
    size_t contains() const { // test only
        return m_set->size();
    }
    size_t size() const; // = cell_count

    bool insert(pk0_type const &);
    bool find(pk0_type const &) const;
    
    template<class fun_type>
    break_or_continue for_each(fun_type &&) const;
private:
	template<class fun_type> static
	break_or_continue for_range(pk0_type, pk0_type, fun_type &&);
};

} // db
} // sdl

#include "dataserver/spatial/interval_set.hpp"

#endif // __SDL_SPATIAL_INTERVAL_SET_H__