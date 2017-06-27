// interval_set.hpp
//
#pragma once
#ifndef __SDL_SPATIAL_INTERVAL_SET_HPP__
#define __SDL_SPATIAL_INTERVAL_SET_HPP__

namespace sdl { namespace db {

template<typename pk0_type>
bool interval_set<pk0_type>::insert_without_size(pk0_type const & cell) {
    set_type & this_set = *m_set;
    auto const rh = this_set.lower_bound(cell);
    if (rh != this_set.end()) {
        if (is_same(*rh, cell)) {
            return false; // already exists
        }
        SDL_ASSERT(is_less(cell, *rh));
        if (rh != this_set.begin()) { // insert in middle of set
            auto const lh = previous(rh);
            SDL_ASSERT(is_less(*lh, cell));
            if (is_interval(*lh)) {
                SDL_ASSERT(!is_interval(*rh));
                return false; // cell is inside interval [lh..rh]
            }
            if (is_next(*lh, cell)) {
                if (end_interval(lh)) {
                    SDL_ASSERT(!is_next(*lh, *rh));
                    if (is_next(cell, *rh)) { // merge [..lh][cell][rh..]
                        if (is_interval(*rh)) { // merge two intervals
                            auto last = rh; ++last;
                            this_set.erase(lh, last);
                        }
                        else {
                            this_set.erase(lh);
                        }
                    }
                    else { // merge [..lh][cell]
                        this_set.insert(this_set.erase(lh), cell);
                    }
                    return true;
                }
                start_interval(lh);
                if (is_next(cell, *rh)) { // merge two intervals
                    if (is_interval(*rh)) {
                        this_set.erase(rh);
                    }
                    return true;
                }
                this_set.insert(rh, cell);
                return true;
            }
        }
        SDL_ASSERT((rh == this_set.begin()) || !is_next(*previous(rh), cell));
        if (is_next(cell, *rh)) { // merge [cell][rh..]
            if (is_interval(*rh))
                insert_interval(this_set.erase(rh), cell);
            else
                insert_interval(rh, cell);
            return true;
        }
    }
    else if (!this_set.empty()) { // insert at end of set
        SDL_ASSERT(rh == this_set.end());
        auto const lh = previous(rh);
        SDL_ASSERT(is_less(*lh, cell));
        if (is_next(*lh, cell)) { // merge interval
            if (end_interval(lh)) {
                this_set.insert(this_set.erase(lh), cell);
                return true;
            }
            start_interval(lh);
        }
    }
    this_set.insert(rh, cell); //use iterator hint when possible
    return true;
}

template<typename pk0_type>
bool interval_set<pk0_type>::find(pk0_type const & cell) const 
{
    auto rh = m_set->lower_bound(cell);
    if (rh != m_set->end()) {
        if (is_same(*rh, cell)) {
            return true;
        }
        if (rh != m_set->begin()) {
            return is_interval(*(--rh));
        }
    }
    return false;
}

template<typename pk0_type>
size_t interval_set<pk0_type>::cell_count() const 
{
    size_t count = 0;
    auto const last = m_set->end();
    bool interval = false;
    pk0_type start = {};
    for (auto it = m_set->begin(); it != last; ++it) {
#if SDL_DEBUG > 1
        if (it != m_set->begin()) {
            auto prev = it; --prev;
            if (is_next(*prev, *it)) {
                SDL_ASSERT(is_interval(*prev));
                SDL_ASSERT(!is_interval(*it));
            }
        }
#endif
        if (is_interval(*it)) {
            SDL_ASSERT(!interval);
            interval = true;
            start = it->key;
        }
        else {
            if (interval) {
                interval = false;
                SDL_ASSERT(start < it->key);
                //SDL_ASSERT(it->key > start);
                count += distance(start, it->key) + 1;
            }
            else {
                ++count;
            }
        }
    }
    return count;
}

template<typename pk0_type>
template<class fun_type> break_or_continue
interval_set<pk0_type>::for_range(pk0_type x1, pk0_type const x2, fun_type && fun) {
    SDL_ASSERT(x1 < x2); //SDL_ASSERT(x1 <= x2);
    while (x1 < x2) {
        if (make_break_or_continue(fun(x1)) == bc::break_) {
            return bc::break_; 
        }
        ++x1;
    }
    SDL_ASSERT(x1 == x2);
    return bc::continue_;
}

template<typename pk0_type>
template<class fun_type> 
typename interval_set<pk0_type>::const_iterator_bc
interval_set<pk0_type>::for_interval(set_const_iterator it, fun_type && fun) const
{
    SDL_ASSERT(it != m_set->end());
    if (is_interval(*it)) {
        const auto x1 = (it++)->key;
        SDL_ASSERT(!is_interval(*it));
        SDL_ASSERT(it != m_set->end());
        const auto x2 = (it++)->key;
        return { it, for_range(x1, x2 + 1, fun) };
    }
    else {
        break_or_continue const b = make_break_or_continue(fun(it->key)); 
        return { ++it, b };
    }
}

template<typename pk0_type>
template<class fun_type> 
typename interval_set<pk0_type>::const_iterator_bc
interval_set<pk0_type>::for_interval2(set_const_iterator it, fun_type && fun) const
{
    SDL_ASSERT(it != m_set->end());
    if (is_interval(*it)) {
        const auto x1 = (it++)->key;
        SDL_ASSERT(!is_interval(*it));
        SDL_ASSERT(it != m_set->end());
        const auto x2 = (it++)->key;
        break_or_continue const b = make_break_or_continue(fun(x1, x2)); 
        return { it, b };
    }
    else {
        break_or_continue const b = make_break_or_continue(fun(it->key, it->key)); 
        return { ++it, b };
    }
}

template<typename pk0_type>
template<class fun_type>
break_or_continue interval_set<pk0_type>::for_each(fun_type && fun) const
{
    static_assert(std::numeric_limits<pk0_type>::is_integer, "");
    auto const last = m_set->cend();
    auto it = m_set->cbegin();
    while (it != last) {
        auto const p = for_interval(it, fun);
        if (p.second == bc::break_) {
            return bc::break_;
        }
        it = p.first;
    }
    return bc::continue_;
}

template<typename pk0_type>
template<class fun_type>
break_or_continue interval_set<pk0_type>::for_each2(fun_type && fun) const
{
    static_assert(std::numeric_limits<pk0_type>::is_integer, "");
    auto const last = m_set->cend();
    auto it = m_set->cbegin();
    while (it != last) {
        auto const p = for_interval2(it, fun);
        if (p.second == bc::break_) {
            return bc::break_;
        }
        it = p.first;
    }
    return bc::continue_;
}

template<typename pk0_type> inline
typename interval_set<pk0_type>::iterator
interval_set<pk0_type>::begin() const
{
    return iterator(this, iterator_state(m_set->begin(), 0));
}

template<typename pk0_type> inline
typename interval_set<pk0_type>::iterator
interval_set<pk0_type>::end() const
{
    return iterator(this, iterator_state(m_set->end(), 0));
}

template<typename pk0_type> inline pk0_type
interval_set<pk0_type>::dereference(iterator_state const & it) const {
    SDL_ASSERT(assert_iterator_state(it));
    if (is_interval(*it.first)) {
        return it.first->key + it.second;
    }
    return it.first->key;
}

template<typename pk0_type>
void interval_set<pk0_type>::load_next(iterator_state & state) const {
    SDL_ASSERT(assert_iterator_state(state));
    if (is_interval(*state.first)) {
        auto it = state.first;
        const auto x1 = (it++)->key;
        ++(state.second);
        if (x1 + state.second < it->key)
            return;
    }
    ++(state.first);
    state.second = 0;
}

template<typename pk0_type>
bool interval_set<pk0_type>::assert_iterator_state(iterator_state const & state) const
{
    static_assert(std::is_integral<pk0_type>::value, "");
    auto it = state.first;
    SDL_ASSERT(it != m_set->end());
    if (is_interval(*it)) {
        const auto x1 = (it++)->key;
        SDL_ASSERT(!is_interval(*it));
        SDL_ASSERT(it != m_set->end());
        const auto x2 = (it++)->key;
        SDL_ASSERT(x1 < x2);
        SDL_ASSERT(x1 + state.second < x2);
        (void)x1;
        (void)x2;
    }
    else {
        SDL_ASSERT(state.second == 0);
    }
    return true;
}

} // db
} // sdl

#endif // __SDL_SPATIAL_INTERVAL_SET_HPP__