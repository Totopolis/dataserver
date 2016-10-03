// interval_cell.h
//
#pragma once
#ifndef __SDL_SYSTEM_INTERVAL_CELL_H__
#define __SDL_SYSTEM_INTERVAL_CELL_H__

#include "spatial_type.h"
#include <set>

namespace sdl { namespace db {

class interval_cell : noncopyable {
    enum { interval_mask = 1 << 4 };
    enum { depth_mask = interval_mask - 1 };
    using value_t = spatial_cell;
    static bool is_interval(value_t const & x) {
        return (x.data.depth & interval_mask) != 0;
    }
    static void set_interval(value_t & x) {
        SDL_ASSERT(!(x.data.depth & interval_mask));
        x.data.depth |= interval_mask;
    }
    static spatial_cell::id_type get_depth(value_t const & x) {
        return x.data.depth & depth_mask;
    }
    static bool is_less(value_t const & x, value_t const & y) {
        SDL_ASSERT(get_depth(x) == get_depth(y));
        return x.r32() < y.r32();
    }
    static bool is_same(value_t const & x, value_t const & y) {
        SDL_ASSERT(get_depth(x) == get_depth(y));
        return x.data.id == y.data.id;
    }
    static bool is_next(value_t const & x, value_t const & y) {
        SDL_ASSERT(get_depth(x) == get_depth(y));
        SDL_ASSERT(x.r32() < y.r32());
        return x.r32() + 1 == y.r32();
    }
    struct key_compare {
        bool operator () (value_t const & x, value_t const & y) const {
            return is_less(x, y);
        }
    };
private:
    using set_type = std::set<value_t, key_compare>;
    using iterator = set_type::iterator;
    using const_iterator = set_type::const_iterator;
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
        SDL_ASSERT(!(it->data.depth & interval_mask));
        set_interval(const_cast<value_t &>(*it)); // mutable depth !
    }
    void insert_interval(iterator const & hint, value_t cell) {
        SDL_ASSERT(m_set->find(cell) == m_set->end());
        set_interval(cell);
        m_set->insert(hint, cell);
    }
    iterator previous(iterator it) {
        SDL_ASSERT(it != m_set->begin());
        return --it;
    }
    using const_iterator_bc = std::pair<const_iterator, break_or_continue>;
    template<class fun_type>
    const_iterator_bc for_interval(const_iterator, fun_type &&) const;
public:
    using cell_ref = spatial_cell const &;
    interval_cell(): m_set(new set_type){
        A_STATIC_ASSERT_IS_POD(value_t);
    }
    interval_cell(interval_cell && src): m_set(std::move(src.m_set)) {}
    void swap(interval_cell & src) {
        m_set.swap(src.m_set);
    }
    const interval_cell & operator=(interval_cell && v) {
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

    void insert(spatial_cell);
    bool find(spatial_cell) const;
    
    template<class fun_type>
    break_or_continue for_each(fun_type &&) const;
    
    template<class cell_fun, class interval_fun>
    break_or_continue for_each_interval(cell_fun &&, interval_fun &&) const;

#if SDL_DEBUG > 1
    void trace(bool);
#endif
private:
	template<spatial_cell::depth_t, class fun_type> static
	break_or_continue for_range(uint32, uint32, fun_type &&);
};

} // db
} // sdl

#include "interval_cell.hpp"

#endif // __SDL_SYSTEM_INTERVAL_CELL_H__