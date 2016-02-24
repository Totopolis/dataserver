// page_iterator.h
//
#ifndef __SDL_SYSTEM_PAGE_ITERATOR_H__
#define __SDL_SYSTEM_PAGE_ITERATOR_H__

#pragma once

#include <iterator>

namespace sdl { namespace db {

/*template<class T>
struct is_same_default {
    enum { value = false };
};

template<class T>
using is_same_default_t = Int2Type<is_same_default<T>::value>;
*/

template<class T, class _value_type, 
    class _category = std::bidirectional_iterator_tag>
class page_iterator : public std::iterator<_category, _value_type>
{
    using state_type = _value_type;
private:
    T * parent;
    state_type current; // must allow iterator assignment

    friend T;
    page_iterator(T * p, state_type && v): parent(p), current(std::move(v)) {
        SDL_ASSERT(parent);
    }
    explicit page_iterator(T * p): parent(p), current() {
        SDL_ASSERT(parent);
    }
    bool is_end() const {
        return parent->is_end(current);
    }
    /*bool is_same(state_type const & it, Int2Type<true>) const {
        return current == it;
    }
    bool is_same(state_type const & it, Int2Type<false>) const {
        return T::is_same(current, it);
    }*/
public:
    page_iterator() : parent(nullptr), current{} {}

    page_iterator & operator++() { // preincrement
        SDL_ASSERT(parent && !is_end());
        parent->load_next(current);
        return (*this);
    }
    page_iterator operator++(int) { // postincrement
        auto temp = *this;
        ++(*this);
        SDL_ASSERT(temp != *this);
        return temp;
    }
    page_iterator & operator--() { // predecrement
        A_STATIC_ASSERT_TYPE(std::bidirectional_iterator_tag, _category);
        SDL_ASSERT(parent);
        parent->load_prev(current);        
        return (*this);
    }
    page_iterator operator--(int) { // postdecrement
        A_STATIC_ASSERT_TYPE(std::bidirectional_iterator_tag, _category);
        auto temp = *this;
        --(*this);
        SDL_ASSERT(temp != *this);
        return temp;
    }
    bool operator==(const page_iterator& it) const {
        SDL_ASSERT(!parent || !it.parent || (parent == it.parent));
        return (parent == it.parent) && 
            T::is_same(current, it.current);
            //is_same(it.current, is_same_default_t<state_type>());
    }
    bool operator!=(const page_iterator& it) const {
        return !(*this == it);
    }
    auto operator*() const -> decltype(parent->dereference(current)) {
        SDL_ASSERT(parent && !is_end());
        return parent->dereference(current);
    }
private:
    void operator->() const = delete; 
};

template<class T, class _value_type>
using forward_iterator = page_iterator<T, _value_type, std::forward_iterator_tag>;

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_ITERATOR_H__
