// datetime.h
//
#pragma once
#ifndef __SDL_COMMON_DATETIME_H__
#define __SDL_COMMON_DATETIME_H__

#include "dataserver/common/common.h"

namespace sdl {

#pragma pack(push, 1) 

struct gregorian_t {
    int year;
    int month;
    int day;
};

struct clocktime_t {
    int hour;  // hours since midnight - [0, 23]
    int min;   // minutes after the hour - [0, 59]
    int sec;   // seconds after the minute - [0, 60] including leap second
    int milliseconds; // < 1 second
};

inline constexpr bool is_empty(gregorian_t const & d) {
    return (d.year == 0) 
        && (d.month == 0)
        && (d.day == 0);
} 
inline constexpr bool is_valid(gregorian_t const & d) { // is_valid if empty
    return (d.year >= 0) && (d.year <= 9999)
        && (d.month >= 0) && (d.month <= 12)
        && (d.day >= 0) && (d.day <= 31);
}

inline constexpr bool operator == (gregorian_t const & x, gregorian_t const & y) {
    return (x.year == y.year)
        && (x.month == y.month)
        && (x.day == y.day);
}

inline constexpr bool operator <(gregorian_t const & x, gregorian_t const & y) {
    return (x.year < y.year) ? true :
           (y.year < x.year) ? false :
           (x.month < y.month) ? true :
           (y.month < x.month) ? false :
           (x.day < y.day);
}

template <class U>
struct quantity_gregorian {
    using unit_type = U;
    using value_type = gregorian_t;
    value_type value;
};

namespace quantity_ {

template <class U>
inline constexpr bool operator == (quantity_gregorian<U> const & x,
                                   quantity_gregorian<U> const & y) {
    return x.value == y.value;
}

} // quantity_

/*
Datetime Data Type

The datetime data type is a packed byte array which is composed of two integers - the number of days since 1900-01-01 (a signed integer value),
and the number of clock ticks since midnight (where each tick is 1/300th of a second), as explored on this blog and this Microsoft article.
http://www.sql-server-performance.com/2004/datetime-datatype/
https://msdn.microsoft.com/en-us/library/aa175784(v=sql.80).aspx

This gives the interesting result that a zero datetime value with all bytes zero is equal to 1900-01-01 at midnight. 
It also tells us that the datetime structure is a very inefficient way to store time (the datetime2 data type was created to address this concern), 
except that it is excellent at defaulting to a reasonable zero point, and that the date and time parts can be split apart very easily by SQL server.
Note that while it is capable of storing days up to the year plus or minus 58 million, it is limited by rule to only go between 1753-01-01 and 9999-12-31.
And note that while the clock ticks part is a 32-bit number, in practice the highest value used will be 25919999.
Since the datatime clock ticks are 1/300ths of a second, while they display accuracy to the millisecond, 
the will actually be rounded to the nearest 0, 3, 7, or 10 millisecond boundary in all conversions and comparisons.
*/
struct datetime_t // 8 bytes
{
    uint32 ticks;   // clock ticks since midnight (where each tick is 1/300th of a second)
    int32 days;     // days since 1900-01-01

    static constexpr int32 u_date_diff = 25567; // = SELECT DATEDIFF(d, '19000101', '19700101');

    // convert to number of seconds that have elapsed since 00:00:00 UTC, 1 January 1970
    size_t get_unix_time() const;
    static datetime_t set_unix_time(size_t);
    static datetime_t current() {
        return datetime_t::set_unix_time(unix_time());
    }
    static datetime_t set_gregorian(gregorian_t const &);
    bool is_null() const {
        return !days && !ticks;
    }
    bool unix_epoch() const {
        return days >= u_date_diff;
    }
    bool before_epoch() const {
        return !unix_epoch();
    }
    static datetime_t init(int32 days, uint32 ticks) {
        datetime_t d;
        d.ticks = ticks;
        d.days = days;
        return d;
    }
    int milliseconds() const {
        return (ticks % 300) * 1000 / 300; // < 1 second
    }
    gregorian_t gregorian() const;
    clocktime_t clocktime() const;
};

#pragma pack(pop)

} // sdl

#endif // __SDL_COMMON_DATETIME_H__