// slot_iterator.h
//
#ifndef __SDL_SYSTEM_SLOT_ITERATOR_H__
#define __SDL_SYSTEM_SLOT_ITERATOR_H__

#pragma once

#include <iterator>

namespace sdl { namespace db {

template<class T>
class slot_iterator : public std::iterator<
    std::bidirectional_iterator_tag, 
    typename T::value_type>
{
    T const * parent;
    size_t slot_index;

    size_t parent_size() const {
        SDL_ASSERT(parent);
        return parent->size();
    }
    
    friend T;
    explicit slot_iterator(T const * p, size_t i = 0)
        : parent(p)
        , slot_index(i)
    {
        SDL_ASSERT(parent);
        SDL_ASSERT(slot_index <= parent->size());
    }
public:
    slot_iterator(): parent(nullptr), slot_index(0) {}

    slot_iterator & operator++() { // preincrement
        SDL_ASSERT(slot_index < parent_size());
        ++slot_index;
        return (*this);
    }
    slot_iterator operator++(int) { // postincrement
        auto temp = *this;
        ++(*this);
        return temp;
    }
    slot_iterator & operator--() { // predecrement
        SDL_ASSERT(slot_index);
        --slot_index;
        return (*this);
    }
    slot_iterator operator--(int) { // postdecrement
        auto temp = *this;
        --(*this);
        return temp;
    }
    bool operator==(const slot_iterator& it) const {
        SDL_ASSERT(!parent || !it.parent || (parent == it.parent));
        return
            (parent == it.parent) &&
            (slot_index == it.slot_index);
    }
    bool operator!=(const slot_iterator& it) const {
        return !(*this == it);
    }    
    value_type operator*() const {
        SDL_ASSERT(slot_index < parent_size());
        A_STATIC_ASSERT_TYPE(value_type, decltype((*parent)[slot_index]));
        return (*parent)[slot_index];
    }
private:
    void operator->() const = delete; 
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SLOT_ITERATOR_H__
