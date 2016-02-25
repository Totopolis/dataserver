// quantity.h
//
#ifndef __SDL_COMMON_QUANTITY_H__
#define __SDL_COMMON_QUANTITY_H__

#pragma once

namespace sdl {

template<class T> struct quantity_traits
{
    enum { allow_increment = false };
    enum { allow_decrement = false };
};

template <class U, class T = int>
struct quantity
{
    typedef quantity<U, T> this_type;
    typedef U unit_type;
    typedef T value_type;
    quantity(): m_value()
    {
        static_assert(sizeof(this_type) == sizeof(m_value), "");
    }
    quantity(value_type x): m_value(x){} // construction from raw value_type is allowed
    value_type value() const
    {
        static_assert(sizeof(value_type) <= sizeof(double), "");
        return m_value;
    }
    quantity& operator++() // prefix
    {
        static_assert(quantity_traits<this_type>::allow_increment, "");
        ++m_value;
        return *this;
    }
    quantity& operator--() // prefix
    {
        static_assert(quantity_traits<this_type>::allow_decrement, "");
        --m_value;
        return *this;
    }
    quantity operator++(int) // postfix
    {
        static_assert(quantity_traits<this_type>::allow_increment, "");
        return quantity(m_value++);
    }
    quantity operator--(int) // postfix
    {
        static_assert(quantity_traits<this_type>::allow_decrement, "");
        return quantity(m_value--);
    }
    bool empty() const
    {
        return (value_type() == m_value);
    }
private:
    enum { force_instantiation_of_unit = sizeof(unit_type)};
    value_type m_value;
};

namespace quantity_ { // protection from unintended ADL

template <class U, class T>
inline bool operator == (quantity<U, T> x, quantity<U, T> y)
{
    return x.value() == y.value();
}

template <class U, class T>
inline bool operator != (quantity<U, T> x, quantity<U, T> y)
{
    return x.value() != y.value();
}

template <class U, class T>
inline bool operator < (quantity<U, T> x, quantity<U, T> y)
{
    return x.value() < y.value();
}

template <class U, class T>
inline bool operator > (quantity<U, T> x, quantity<U, T> y)
{
    return x.value() > y.value();
}

template <class U, class T>
inline bool operator <= (quantity<U, T> x, quantity<U, T> y)
{
    return x.value() <= y.value();
}

template <class U, class T>
inline bool operator >= (quantity<U, T> x, quantity<U, T> y)
{
    return x.value() >= y.value();
}

} // quantity_

template<class U, class T> inline
std::ostream & operator <<(std::ostream & out, quantity<U, T> const & i) {
    out << i.value();
    return out;
}

} // sdl

#endif // __SDL_COMMON_QUANTITY_H__