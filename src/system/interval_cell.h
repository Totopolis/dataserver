// interval_cell.h
//
#pragma once
#ifndef __SDL_SYSTEM_INTERVAL_CELL_H__
#define __SDL_SYSTEM_INTERVAL_CELL_H__

#include "spatial_type.h"
#include <set>

namespace sdl { namespace db {

class interval_cell : noncopyable { // movable
private:
    static bool is_less(spatial_cell const & x, spatial_cell const & y) {
        return x.r32() < y.r32();
    }
    static bool is_same(spatial_cell const & x, spatial_cell const & y) {
        return x.data.id._32 == y.data.id._32;
    }
    static bool is_next(spatial_cell const & x, spatial_cell const & y) {
        SDL_ASSERT(x.r32() < y.r32());
        return x.r32() + 1 == y.r32();
    }
    struct key_compare {
        bool operator () (spatial_cell const & x, spatial_cell const & y) const {
            return is_less(x, y);
        }
    };
private:
    static const uint8 zero_depth = 0; // token of start interval
    using set_type = std::set<spatial_cell, key_compare>;
    using iterator = set_type::iterator;
    using const_iterator = set_type::const_iterator;
    std::unique_ptr<set_type> m_set;
private:
    static bool is_interval(spatial_cell const & x) {
        return x.data.depth == zero_depth;
    }
    bool end_interval(iterator const & it) const {
        if (it != m_set->begin()) {
            iterator p = it;
            return is_interval(*(--p));
        }
        return false;
    }
    void insert_interval(iterator const & hint, spatial_cell const & cell) {
        SDL_ASSERT(m_set->find(cell) == m_set->end());
        m_set->insert(hint, spatial_cell::init(cell.data.id._32, zero_depth));
    }
    void start_interval(iterator const & it) {
        const_cast<spatial_cell &>(*it).data.depth = zero_depth; 
    }
    iterator previous(iterator it) {
        SDL_ASSERT(it != m_set->begin());
        return --it;
    }
    using const_iterator_bc = std::pair<const_iterator, break_or_continue>;
    template<class fun_type>
    const_iterator_bc for_interval(const_iterator, fun_type) const;
public:
    interval_cell(): m_set(new set_type){}
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
    size_t set_size() const { // test only
        return m_set->size();
    }
    void clear() {
        m_set->clear();
    }
    size_t cell_count() const;
    void insert(spatial_cell const &);
    
    template<class fun_type>
    break_or_continue for_each(fun_type) const;

#if SDL_DEBUG
    void trace(bool);
#endif
};

template<class fun_type>
interval_cell::const_iterator_bc
interval_cell::for_interval(const_iterator it, fun_type fun) const {
    SDL_ASSERT(it != m_set->end());
    if (is_interval(*it)) {
        SDL_ASSERT(it->data.depth == 0);
        const uint32 x1 = (it++)->r32();
        SDL_ASSERT(it->data.depth == 4);
        SDL_ASSERT(it != m_set->end());
        const uint32 x2 = (it++)->r32();
        for (uint32 x = x1; x <= x2; ++x) {
            if (fun(spatial_cell::init(reverse_bytes(x))) == bc::break_) {
                return { it, bc::break_ }; 
            }
        }
        return { it, bc::continue_ };
    }
    else {
        SDL_ASSERT(it->data.depth == 4);
        break_or_continue const b = fun(*it++);
        return { it, b };
    }
}

template<class fun_type>
break_or_continue interval_cell::for_each(fun_type fun) const {
    auto const last = m_set->end();
    auto it = m_set->begin();
    while (it != last) {
        auto const p = for_interval(it, fun);
        if (p.second == bc::break_) {
            return bc::break_;
        }
        it = p.first;
    }
    return bc::continue_;
}

} // db
} // sdl

#endif // __SDL_SYSTEM_INTERVAL_CELL_H__