// static_type.h
//
#pragma once
#ifndef __SDL_COMMON_STATIC_TYPE_H__
#define __SDL_COMMON_STATIC_TYPE_H__

#include "dataserver/common/static.h"

namespace sdl {

template<class T1, class T2>
struct first_second {
    T1 first;
    T2 second;
};

////////////////////////////////////////////////////////////////////////////////

template <int v> struct Int2Type
{
    enum { value = v };
};

template <size_t v> struct Size2Type
{
    static constexpr size_t value = v;
};

template <typename T, T v> struct Val2Type
{
    using type = T;
    static T constexpr value = v;
};

template <typename T> struct Type2Type
{
    typedef T OriginalType;
};

struct NullType {};

template <typename T>
struct IsNullType
{
    enum { value = false };
};

template <>
struct IsNullType<NullType>
{
    enum { value = true };
};

////////////////////////////////////////////////////////////////////////////////
// class template Typelist
// The building block of typelists of any length
// Use it through the A_TYPELIST_NN macros
// Defines nested types:
//     Head (first element, a non-typelist type by convention)
//     Tail (second element, can be another typelist)
////////////////////////////////////////////////////////////////////////////////

template <class T, class U>
struct Typelist
{
    typedef T Head;
    typedef U Tail;
};

////////////////////////////////////////////////////////////////////////////////

template <class T>
struct IsTypelist
{
    enum { value = false };
    A_STATIC_ASSERT_NOT_TYPE(T, NullType);
};

template <> struct IsTypelist<NullType>
{
    enum { value = true };
};

template <class Head, class Tail>
struct IsTypelist<Typelist<Head, Tail>>
{
    enum { value = true };
    A_STATIC_ASSERT_NOT_TYPE(Head, NullType);
};

////////////////////////////////////////////////////////////////////////////////

template <class T> struct Typelist1 {
    using Result = Typelist<T, NullType>;
    static_assert(!IsTypelist<T>::value, "");
};

template <> struct Typelist1<NullType> {
    using Result = NullType;
};

template <class T>
using Typelist1_t = typename Typelist1<T>::Result;

////////////////////////////////////////////////////////////////////////////////
// class template Select
// Selects one of two types based upon a boolean constant
// Invocation: Select<flag, T, U>::Result
// where:
// flag is a compile-time boolean constant
// T and U are types
// Result evaluates to T if flag is true, and to U otherwise.
////////////////////////////////////////////////////////////////////////////////

template <bool flag, typename T, typename U>
struct Select
{
    typedef T Result;
};
template <typename T, typename U>
struct Select<false, T, U>
{
    typedef U Result;
};

template <bool flag, typename T, typename U>
using Select_t = typename Select<flag, T, U>::Result;

////////////////////////////////////////////////////////////////////////////////
// class template IsSameType
// Return true iff two given types are the same
// Invocation: SameType<T, U>::value
// where:
// T and U are types
// Result evaluates to true iff U == T (types equal)
////////////////////////////////////////////////////////////////////////////////

template <typename T, typename U>
struct IsSameType
{
    enum { value = false };
};

template <typename T>
struct IsSameType<T, T>
{
    enum { value = true };
};

namespace TL {

////////////////////////////////////////////////////////////////////////////////
// class template Length
// Computes the length of a typelist
// Invocation (TList is a typelist):
// Length<TList>::value
// returns a compile-time constant containing the length of TList, not counting
//     the end terminator (which by convention is NullType)
////////////////////////////////////////////////////////////////////////////////

template <class TList> struct Length;
template <> struct Length<NullType>
{
    enum { value = 0 };
};

template <class T, class U>
struct Length< Typelist<T, U> >
{
    enum { value = 1 + Length<U>::value };
};

////////////////////////////////////////////////////////////////////////////////

template <class TList> struct IsEmpty
{
    enum { value = Length<TList>::value == 0 };
};

////////////////////////////////////////////////////////////////////////////////

template <class TList> struct TypeFirst;
template <> struct TypeFirst<NullType> {
    using Result = NullType;
};

template <class Head, class Tail>
struct TypeFirst<Typelist<Head, Tail>> {
    using Result = Head;
};

template <class TList>
using TypeFirst_t = typename TypeFirst<TList>::Result;

////////////////////////////////////////////////////////////////////////////////

template <class TList> struct TypeLast;
template <> struct TypeLast<NullType> {
    using Result = NullType;
};

template <class Head>
struct TypeLast<Typelist<Head, NullType>> {
    using Result = Head;
};

template <class Head, class Tail>
struct TypeLast<Typelist<Head, Tail>> {
    using Result = typename TypeLast<Tail>::Result;
};

template <class TList>
using TypeLast_t = typename TypeLast<TList>::Result;

////////////////////////////////////////////////////////////////////////////////
// class template TypeAt
// Finds the type at a given index in a typelist
// Invocation (TList is a typelist and index is a compile-time integral 
//     constant):
// TypeAt<TList, index>::Result
// returns the type in position 'index' in TList
// If you pass an out-of-bounds index, the result is a compile-time error
////////////////////////////////////////////////////////////////////////////////

template <class TList, unsigned int index> struct TypeAt;

template <class Head, class Tail>
struct TypeAt<Typelist<Head, Tail>, 0>
{
    typedef Head Result;
};

template <class Head, class Tail, unsigned int i>
struct TypeAt<Typelist<Head, Tail>, i>
{
    typedef typename TypeAt<Tail, i - 1>::Result Result;
};

////////////////////////////////////////////////////////////////////////////////
// class template TypeAtNonStrict
// Finds the type at a given index in a typelist
// Invocations (TList is a typelist and index is a compile-time integral 
//     constant):
// a) TypeAt<TList, index>::Result
// returns the type in position 'index' in TList, or NullType if index is 
//     out-of-bounds
// b) TypeAt<TList, index, D>::Result
// returns the type in position 'index' in TList, or D if index is out-of-bounds
////////////////////////////////////////////////////////////////////////////////

template <class TList, unsigned int index, typename DefaultType = NullType>
struct TypeAtNonStrict
{
    typedef DefaultType Result;
};

template <class Head, class Tail, typename DefaultType>
struct TypeAtNonStrict<Typelist<Head, Tail>, 0, DefaultType>
{
    typedef Head Result;
};

template <class Head, class Tail, unsigned int i, typename DefaultType>
struct TypeAtNonStrict<Typelist<Head, Tail>, i, DefaultType>
{
    typedef typename TypeAtNonStrict<Tail, i - 1, DefaultType>::Result Result;
};

////////////////////////////////////////////////////////////////////////////////
// class template IndexOf
// Finds the index of a type in a typelist
// Invocation (TList is a typelist and T is a type):
// IndexOf<TList, T>::value
// returns the position of T in TList, or NullType if T is not found in TList
////////////////////////////////////////////////////////////////////////////////

template <class TList, class T> struct IndexOf;

template <class T>
struct IndexOf<NullType, T>
{
    enum { value = -1 };
};

template <class T, class Tail>
struct IndexOf<Typelist<T, Tail>, T>
{
    enum { value = 0 };
};

template <class Head, class Tail, class T>
struct IndexOf<Typelist<Head, Tail>, T>
{
private:
    enum { temp = IndexOf<Tail, T>::value };
public:
    enum { value = (temp == -1 ? -1 : 1 + temp) };
};

////////////////////////////////////////////////////////////////////////////////
// class template Append
// Appends a type or a typelist to another
// Invocation (TList is a typelist and T is either a type or a typelist):
// Append<TList, T>::Result
// returns a typelist that is TList followed by T and NullType-terminated
////////////////////////////////////////////////////////////////////////////////
template <class TList, class T> struct Append;

template <> struct Append<NullType, NullType>
{
    typedef NullType Result;
};

template <class T> struct Append<NullType, T>
{
    typedef Typelist<T, NullType> Result;
};

template <class Head, class Tail>
struct Append<NullType, Typelist<Head, Tail> >
{
    typedef Typelist<Head, Tail> Result;
};

template <class Head, class Tail, class T>
struct Append<Typelist<Head, Tail>, T>
{
    typedef Typelist<Head, 
            typename Append<Tail, T>::Result>
        Result;
};

template <class TList, class T>
using Append_t = typename Append<TList, T>::Result;

////////////////////////////////////////////////////////////////////////////////
// class template Erase
// Erases the first occurence, if any, of a type in a typelist
// Invocation (TList is a typelist and T is a type):
// Erase<TList, T>::Result
// returns a typelist that is TList without the first occurence of T
////////////////////////////////////////////////////////////////////////////////

template <class TList, class T> struct Erase;

template <class T>                         // Specialization 1
struct Erase<NullType, T>
{
    typedef NullType Result;
};

template <class T, class Tail>             // Specialization 2
struct Erase<Typelist<T, Tail>, T>
{
    typedef Tail Result;
};

template <class Head, class Tail, class T> // Specialization 3
struct Erase<Typelist<Head, Tail>, T>
{
    typedef Typelist<Head, 
            typename Erase<Tail, T>::Result>
        Result;
};

template <class TList, class T>
using Erase_t = typename Erase<TList, T>::Result;

////////////////////////////////////////////////////////////////////////////////
// class template EraseAll
// Erases all first occurences, if any, of a type in a typelist
// Invocation (TList is a typelist and T is a type):
// EraseAll<TList, T>::Result
// returns a typelist that is TList without any occurence of T
////////////////////////////////////////////////////////////////////////////////

template <class TList, class T> struct EraseAll;
template <class T>
struct EraseAll<NullType, T>
{
    typedef NullType Result;
};
template <class T, class Tail>
struct EraseAll<Typelist<T, Tail>, T>
{
    // Go all the way down the list removing the type
    typedef typename EraseAll<Tail, T>::Result Result;
};
template <class Head, class Tail, class T>
struct EraseAll<Typelist<Head, Tail>, T>
{
    // Go all the way down the list removing the type
    typedef Typelist<Head, 
            typename EraseAll<Tail, T>::Result>
        Result;
};

////////////////////////////////////////////////////////////////////////////////
// class template NoDuplicates
// Removes all duplicate types in a typelist
// Invocation (TList is a typelist):
// NoDuplicates<TList, T>::Result
////////////////////////////////////////////////////////////////////////////////

template <class TList> struct NoDuplicates;

template <> struct NoDuplicates<NullType>
{
    typedef NullType Result;
};

template <class Head, class Tail>
struct NoDuplicates< Typelist<Head, Tail> >
{
private:
    typedef typename NoDuplicates<Tail>::Result L1;
    typedef typename Erase<L1, Head>::Result L2;
public:
    typedef Typelist<Head, L2> Result;
};

////////////////////////////////////////////////////////////////////////////////

template <class TList>
struct IsDistinct {
private:
    using Temp = typename NoDuplicates<TList>::Result;
public:
    enum { value = Length<TList>::value == Length<Temp>::value };
};

////////////////////////////////////////////////////////////////////////////////
// class template Replace
// Replaces the first occurence of a type in a typelist, with another type
// Invocation (TList is a typelist, T, U are types):
// Replace<TList, T, U>::Result
// returns a typelist in which the first occurence of T is replaced with U
////////////////////////////////////////////////////////////////////////////////

template <class TList, class T, class U> struct Replace;

template <class T, class U>
struct Replace<NullType, T, U>
{
    typedef NullType Result;
};

template <class T, class Tail, class U>
struct Replace<Typelist<T, Tail>, T, U>
{
    typedef Typelist<U, Tail> Result;
};

template <class Head, class Tail, class T, class U>
struct Replace<Typelist<Head, Tail>, T, U>
{
    typedef Typelist<Head,
            typename Replace<Tail, T, U>::Result>
        Result;
};

////////////////////////////////////////////////////////////////////////////////
// class template ReplaceAll
// Replaces all occurences of a type in a typelist, with another type
// Invocation (TList is a typelist, T, U are types):
// Replace<TList, T, U>::Result
// returns a typelist in which all occurences of T is replaced with U
////////////////////////////////////////////////////////////////////////////////

template <class TList, class T, class U> struct ReplaceAll;

template <class T, class U>
struct ReplaceAll<NullType, T, U>
{
    typedef NullType Result;
};

template <class T, class Tail, class U>
struct ReplaceAll<Typelist<T, Tail>, T, U>
{
    typedef Typelist<U, typename ReplaceAll<Tail, T, U>::Result> Result;
};

template <class Head, class Tail, class T, class U>
struct ReplaceAll<Typelist<Head, Tail>, T, U>
{
    typedef Typelist<Head,
            typename ReplaceAll<Tail, T, U>::Result>
        Result;
};

////////////////////////////////////////////////////////////////////////////////
// class template Reverse
// Reverses a typelist
// Invocation (TList is a typelist):
// Reverse<TList>::Result
// returns a typelist that is TList reversed
////////////////////////////////////////////////////////////////////////////////

template <class TList> struct Reverse;

template <>
struct Reverse<NullType>
{
    typedef NullType Result;
};

template <class Head, class Tail>
struct Reverse< Typelist<Head, Tail> >
{
    typedef typename Append<
        typename Reverse<Tail>::Result, Head>::Result Result;
};

////////////////////////////////////////////////////////////////////////////////

/*template<class TList, class T>
struct is_found {
enum { value = TL::IndexOf<TList, T>::value != -1 };
typedef std::integral_constant<bool, value> type;
};*/

} // namespace TL
} // namespace sdl

#endif // __SDL_COMMON_STATIC_TYPE_H__

#if 0
template<class T1, class T2> struct make_pair;

template<> struct make_pair<NullType, NullType> {
    using Result = NullType;
};

template<class T> struct make_pair<NullType, T> {
    using Result = NullType;
};

template<class T> struct make_pair<T, NullType> {
    using Result = NullType;
};

template<class Head, class Tail>
struct make_pair {
    using Result = Typelist<Head, Typelist<Tail, NullType>>;
    static_assert(TL::Length<Result>::value == 2, "");
};

template<class T1, class T2>
using make_pair_t = typename make_pair<T1, T2>::Result;
#endif