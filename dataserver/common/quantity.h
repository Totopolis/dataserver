// quantity.h
//
#pragma once
#ifndef __SDL_COMMON_QUANTITY_H__
#define __SDL_COMMON_QUANTITY_H__

namespace sdl {

template<class T> struct quantity_traits {
    enum { allow_increment = false };
    enum { allow_decrement = false };
};

template <class UNIT_TYPE, class VALUE_TYPE>
class quantity {
public:
    using this_type = quantity<UNIT_TYPE, VALUE_TYPE>;
    using unit_type = UNIT_TYPE;
    using value_type = VALUE_TYPE;

    constexpr quantity(value_type x): m_value(x) { // construction from raw value_type is allowed
        static_assert(sizeof(this_type) == sizeof(value_type), "");
    }
    constexpr bool empty() const {
        return (value_type() == m_value);
    }
    constexpr value_type value() const {
        static_assert(sizeof(value_type) <= sizeof(double), "");
        return m_value;
    }
    quantity& operator++() { // prefix
        static_assert(quantity_traits<this_type>::allow_increment, "");
        ++m_value;
        return *this;
    }
    quantity& operator--() { // prefix
        static_assert(quantity_traits<this_type>::allow_decrement, "");
        --m_value;
        return *this;
    }
    quantity operator++(int) { // postfix
        static_assert(quantity_traits<this_type>::allow_increment, "");
        return quantity(m_value++);
    }
    quantity operator--(int) { // postfix
        static_assert(quantity_traits<this_type>::allow_decrement, "");
        return quantity(m_value--);
    }
    quantity& operator+=(const quantity & other) {
        static_assert(quantity_traits<this_type>::allow_increment, "");
        m_value += other.m_value;
        return *this;
    }
    quantity& operator-=(const quantity & other) {
        static_assert(quantity_traits<this_type>::allow_decrement, "");
        m_value -= other.m_value;
        return *this;
    }
private:
    enum { force_instantiation_of_unit = sizeof(unit_type)};
    value_type m_value;
};

namespace quantity_ { // protection from unintended ADL

template <class U, class T>
inline bool operator == (quantity<U, T> x, quantity<U, T> y) {
    return x.value() == y.value();
}
template <class U, class T>
inline bool operator != (quantity<U, T> x, quantity<U, T> y) {
    return x.value() != y.value();
}
template <class U, class T>
inline bool operator < (quantity<U, T> x, quantity<U, T> y) {
    return x.value() < y.value();
}
template <class U, class T>
inline bool operator > (quantity<U, T> x, quantity<U, T> y) {
    return x.value() > y.value();
}
template <class U, class T>
inline bool operator <= (quantity<U, T> x, quantity<U, T> y) {
    return x.value() <= y.value();
}
template <class U, class T>
inline bool operator >= (quantity<U, T> x, quantity<U, T> y) {
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