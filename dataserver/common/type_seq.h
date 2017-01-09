// type_seq.h
//
#pragma once
#ifndef __SDL_COMMON_TYPE_SEQ_H__
#define __SDL_COMMON_TYPE_SEQ_H__

#include "dataserver/common/static_type.h"

namespace sdl { namespace TL {

#if 0 // before C++11
template
<
    class T01=NullType,class T02=NullType,class T03=NullType,class T04=NullType,class T05=NullType,
    class T06=NullType,class T07=NullType,class T08=NullType,class T09=NullType,class T10=NullType,
    class T11=NullType,class T12=NullType,class T13=NullType,class T14=NullType,class T15=NullType,
    class T16=NullType,class T17=NullType,class T18=NullType,class T19=NullType,class T20=NullType,
    class T21=NullType,class T22=NullType,class T23=NullType,class T24=NullType,class T25=NullType,
    class T26=NullType,class T27=NullType,class T28=NullType,class T29=NullType,class T30=NullType
>
struct Seq
{
private:
    typedef typename Seq<     T02, T03, T04, T05, T06, T07, T08, T09, T10,
                            T11, T12, T13, T14, T15, T16, T17, T18, T19, T20,
                            T21, T22, T23, T24, T25, T26, T27, T28, T29, T30>::Type
                        TailResult;
public:
    typedef Typelist<T01, TailResult> Type;
};
        
template<> struct Seq<>
{
    typedef NullType Type;
};
#endif

template <typename...> struct Seq;
    
template<> struct Seq<> {
    typedef NullType Type;
};

template <typename Head, typename... Types>
struct Seq<Head, Types...> {
    using Type = Typelist<Head, typename Seq<Types...>::Type>;
};

template <typename... T>
using Seq_t = typename Seq<T...>::Type;

} // namespace TL
} // namespace sdl

#endif // __SDL_COMMON_TYPE_SEQ_H__
