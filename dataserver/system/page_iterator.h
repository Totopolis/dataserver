// page_iterator.h
//
#pragma once
#ifndef __SDL_SYSTEM_PAGE_ITERATOR_H__
#define __SDL_SYSTEM_PAGE_ITERATOR_H__

#include <iterator>

namespace sdl { namespace db { namespace page_iterator_ {

struct is_same_delegate_ {
private:
    template<typename T, typename X>
    static auto check(X && x1, X && x2) -> decltype(T::is_same(x1, x2));
    template<typename T> static void check(...);
    template<typename X> static X && make();
public:
    template<typename T, typename X> 
    static auto test() -> decltype(check<T>(make<X>(), make<X>()));
};

template<typename T, typename X> 
using is_same_delegate = identity<decltype(is_same_delegate_::test<T, X>())>;

//------------------------------------------------------------------

struct is_end_delegate_ {
private:
    template<typename T, typename X>
    static auto check(T * p, X && x) -> decltype(p->is_end(x));
    template<typename T> static void check(...);
    template<typename X> static X && make();
public:
    template<typename T, typename X> 
    static auto test() -> decltype(check<T>(nullptr, make<X>()));
};

template<typename T, typename X> 
using is_end_delegate = identity<decltype(is_end_delegate_::test<T, X>())>;

} // page_iterator_

//------------------------------------------------------------------

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
    static bool is_end(identity<void>) {
        return false;
    }
    bool is_end(identity<bool>) const {
        return parent->is_end(current);
    }
    bool is_end() const {
        return is_end(page_iterator_::is_end_delegate<T, state_type>());
    }
    bool is_same(const state_type& it, identity<void>) const {
        return current == it;
    }
    bool is_same(const state_type& it, identity<bool>) const {
        return T::is_same(current, it);
    }
    bool is_same(const state_type& it) const {
        return is_same(it, page_iterator_::is_same_delegate<T, state_type>());
    }
    static page_iterator const & make_this(); // without implementation
public:
    page_iterator() : parent(nullptr), current{} {}

    page_iterator & operator++() { // preincrement
        SDL_ASSERT(!is_end());
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
        return (parent == it.parent) && this->is_same(it.current);
    }
    bool operator!=(const page_iterator& it) const {
        return !(*this == it);
    }
    auto operator*() const -> decltype(parent->dereference(current)) {
        SDL_ASSERT(!is_end());
        return parent->dereference(current);
    }
    using value_type = decltype(*page_iterator::make_this()); //experimental
private:
    void operator->() const = delete; 
};

template<class T, class _value_type>
using forward_iterator = page_iterator<T, _value_type, std::forward_iterator_tag>;

} // db
} // sdl

#endif // __SDL_SYSTEM_PAGE_ITERATOR_H__
