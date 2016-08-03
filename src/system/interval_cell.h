// interval_cell.h
//
#pragma once
#ifndef __SDL_SYSTEM_INTERVAL_CELL_H__
#define __SDL_SYSTEM_INTERVAL_CELL_H__

#include "spatial_type.h"
#include <set>

namespace sdl { namespace db {

//FIXME: allow to insert cells with different depth ?
class interval_cell : noncopyable {
private:
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
    const_iterator_bc for_interval(const_iterator, fun_type) const;
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
    size_t set_size() const { // test only
        return m_set->size();
    }
    void clear() {
        m_set->clear();
    }
    size_t cell_count() const;

    void insert(spatial_cell);
    
    template<class fun_type>
    break_or_continue for_each(fun_type) const;
    
    //FIXME: break_or_continue for_each(cell_fun, interval_fun) const;

    bool find(spatial_cell) const;

#if SDL_DEBUG
    void trace(bool);
#endif
};

template<class fun_type>
interval_cell::const_iterator_bc
interval_cell::for_interval(const_iterator it, fun_type fun) const
{
    SDL_ASSERT(it != m_set->end());
    SDL_ASSERT(get_depth(*it) == spatial_cell::size);
    if (is_interval(*it)) {
        const uint32 x1 = (it++)->r32();
        SDL_ASSERT(!is_interval(*it));
        SDL_ASSERT(it != m_set->end());
        const uint32 x2 = (it++)->r32();
        for (uint32 x = x1; x <= x2; ++x) {
            if (make_break_or_continue(fun(
                spatial_cell::init(reverse_bytes(x), spatial_cell::size))) == bc::break_) {
                return { it, bc::break_ }; 
            }
        }
        return { it, bc::continue_ };
    }
    else {
        SDL_ASSERT(it->data.depth == spatial_cell::size);
        break_or_continue const b = make_break_or_continue(fun(*it)); 
        return { ++it, b };
    }
}

template<class fun_type>
break_or_continue interval_cell::for_each(fun_type fun) const
{
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