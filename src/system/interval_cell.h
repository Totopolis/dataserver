// interval_cell.h
//
#pragma once
#ifndef __SDL_SYSTEM_INTERVAL_CELL_H__
#define __SDL_SYSTEM_INTERVAL_CELL_H__

#include "spatial_type.h"
#include <set>

namespace sdl { namespace db {

class interval_cell: noncopyable { //FIXME: make movable
public:
    struct less {
        bool operator () (spatial_cell const & x, spatial_cell const & y) const {
            return x.data.id.r32() < y.data.id.r32();
        }
    };
private:
    static const uint8 zero_depth = 0;
    using set_type = std::set<spatial_cell, less>;
    using iterator = set_type::iterator;
    using const_iterator = set_type::const_iterator;
    set_type m_set;
private:
    static bool is_same(spatial_cell const & x, spatial_cell const & y) {
        return x.data.id._32 == y.data.id._32;
    }
    static bool is_next(spatial_cell const & x, spatial_cell const & y) {
        const uint32 x1 = x.data.id.r32();
        const uint32 y1 = y.data.id.r32();
        SDL_ASSERT(x1 < y1);
        return x1 + 1 == y1;
    }
    static bool is_interval(spatial_cell const & x) {
        return zero_depth == x.data.depth;
    }
    bool end_interval(iterator) const;
    bool insert_interval(uint32);
    bool insert_interval(spatial_cell const & cell) {
        return insert_interval(cell.data.id._32);
    }
    void start_interval(iterator &);
public:
    interval_cell() = default;

    bool empty() const {
        return m_set.empty();
    }
    size_t cell_count() const;

    bool insert(spatial_cell const &);
    
    template<class fun_type>
    break_or_continue for_each(fun_type) const;
private:
    const_iterator begin() const {
        return m_set.begin();
    }
    const_iterator end() const {
        return m_set.end();
    }
    bool is_interval(const_iterator it) const {
        SDL_ASSERT(it != m_set.end());
        return is_interval(*it);
    }
    using const_iterator_bc = std::pair<const_iterator, break_or_continue>;
    template<class fun_type>
    const_iterator_bc for_interval(const_iterator, fun_type) const;

#if SDL_DEBUG
public:
    void trace(bool);
#endif
};

inline bool interval_cell::end_interval(iterator it) const {
    if (it != m_set.begin()) {
        return is_interval(*(--it));
    }
    return false;
}

inline bool interval_cell::insert_interval(uint32 const _32) {
    bool const ret = m_set.insert(spatial_cell::init(_32, zero_depth)).second;
    SDL_ASSERT(ret);
    return ret;
}

inline void interval_cell::start_interval(iterator & it) { // invalidates iterator
    auto const _32 = it->data.id._32;
    m_set.erase(it);
    insert_interval(_32);
}

template<class fun_type>
interval_cell::const_iterator_bc
interval_cell::for_interval(const_iterator it, fun_type fun) const {
    SDL_ASSERT(it != m_set.end());
    if (is_interval(it)) {
        SDL_ASSERT(it->data.depth == 0);
        const uint32 x1 = (it++)->data.id.r32();
        SDL_ASSERT(it->data.depth == 4);
        SDL_ASSERT(it != m_set.end());
        const uint32 x2 = (it++)->data.id.r32();
        for (uint32 x = x1; x <= x2; ++x) {
            if (fun(spatial_cell::init(reverse_bytes(x), spatial_cell::size)) == bc::break_) {
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
    auto const last = m_set.end();
    auto it = m_set.begin();
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