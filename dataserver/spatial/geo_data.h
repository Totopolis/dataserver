// geo_data.h
//
#pragma once
#ifndef __SDL_SYSTEM_GEO_DATA_H__
#define __SDL_SYSTEM_GEO_DATA_H__

#include "system/page_head.h"
#include "spatial_type.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct geo_data_meta;
struct geo_data_info;

struct geo_data { // 6 bytes

    using meta = geo_data_meta;
    using info = geo_data_info;

    struct data_type {
        uint32       SRID;       // 0x00 : 4 bytes // E6100000 = 4326 (WGS84 — SRID 4326)
        spatial_tag  tag;        // 0x04 : 2 bytes // = TYPEID
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

using geo_head = geo_data::data_type;

//------------------------------------------------------------------------

struct geo_pointarray;
struct geo_tail { // 15 bytes

    struct num_type {       // 5 bytes
        uint32 num;         // value
        uint8 tag;          // ?
    };
    struct data_type {
        num_type numobj;     // 1600000001 = num_lines (22 = 0x16)
        num_type reserved;   // 0000000001
        num_type points[1];  // points offset
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };    
    size_t size() const { 
        return data.numobj.num;
    }
    num_type get(size_t const i) const {
        SDL_ASSERT(i + 1 < this->size());
        return data.points[i];
    }
    template<size_t i> size_t get_num() const {
        SDL_ASSERT(i + 1 < this->size());
        return data.points[i].num;
    }
    size_t operator[](size_t const i) const {
        SDL_ASSERT(i + 1 < this->size());
        return data.points[i].num;
    }
    size_t data_mem_size() const {
        return sizeof(data_type) + sizeof(num_type) * size() - sizeof(data.points);
    }
    spatial_point const * begin(geo_pointarray const &, size_t const subobj) const;
    spatial_point const * end(geo_pointarray const &, size_t const subobj) const;

    template<size_t const subobj> spatial_point const * begin(geo_pointarray const &) const;
    template<size_t const subobj> spatial_point const * end(geo_pointarray const &) const;
};

//------------------------------------------------------------------------

struct geo_point_meta;
struct geo_point_info;

struct geo_point { // 22 bytes
    
    using meta = geo_point_meta;
    using info = geo_point_info;

    static constexpr spatial_type this_type = spatial_type::point;

    struct data_type {
        geo_head      head;     // 0x00 : 6 bytes
        spatial_point point;    // 0x06 : 16 bytes
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    static constexpr size_t size() { 
        return 1;
    } 
    spatial_point const & operator[](size_t i) const {
        SDL_ASSERT(i < this->size());
        return data.point;
    }
    spatial_point const * begin() const {
        return &(data.point);
    }
    spatial_point const * end() const {
        return begin() + this->size();
    }
    static constexpr size_t data_mem_size() {
        return sizeof(data_type);
    }
    bool is_equal(spatial_point const & p) const {
        return data.point == p;
    }
};

//------------------------------------------------------------------------

struct geo_pointarray_meta;
struct geo_pointarray_info;

struct geo_pointarray { // = 26 bytes

    using meta = geo_pointarray_meta;
    using info = geo_pointarray_info;

    struct data_type {
        geo_head        head;       // 0x00 : 6 bytes
        uint32          num_point;  // 0x06 : 4 bytes // EC010000 = 0x01EC = 492 = POINTS COUNT
        spatial_point   points[1];  // 0x0A : 16 bytes * point_num
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    size_t size() const { 
        return data.num_point;
    } 
    spatial_point const & operator[](size_t i) const {
        SDL_ASSERT(i < this->size());
        return data.points[i];
    }
    spatial_point const * begin() const {
        return data.points;
    }
    spatial_point const * end() const {
        return data.points + this->size();
    }
    spatial_point const & front() const {
        return * begin();
    }
    spatial_point const & back() const {
        return * (end() - 1);
    }
    size_t data_mem_size() const {
        return sizeof(data_type) + sizeof(spatial_point) * size() - sizeof(data.points);
    }
    geo_tail const * tail(const size_t data_size) const {
        const auto sz = data_mem_size();
        if (data_size >= (sz + sizeof(geo_tail))) {
            return reinterpret_cast<geo_tail const *>(this->end());
        }
        SDL_ASSERT(0); // to be tested
        return nullptr;
    }
    bool contains(spatial_point const & p) const {
        for (auto const & d : *this) {
            if (d == p)
                return true;
        }
        return false;
    }
};

struct geo_linestring : geo_pointarray { // = 26 bytes
    static constexpr spatial_type this_type = spatial_type::linestring;
};

struct geo_multilinestring : geo_pointarray { // = 26 bytes
    static constexpr spatial_type this_type = spatial_type::multilinestring;
};

//------------------------------------------------------------------------

struct geo_base_polygon : geo_pointarray { // = 26 bytes

    static constexpr spatial_type this_type = spatial_type::polygon;

    bool ring_empty() const;
    size_t ring_num() const;

    template<class fun_type>
    size_t for_ring(fun_type fun) const;
};

template<class fun_type>
size_t geo_base_polygon::for_ring(fun_type fun) const
{
    SDL_ASSERT(size() != 1);
    size_t ring_n = 0;
    auto const _end = this->end();
    auto p1 = this->begin();
    auto p2 = p1 + 1;
    while (p2 < _end) {
        SDL_ASSERT(p1 < p2);
        if (*p1 == *p2) {
            ++ring_n;
            ++p2;
            fun(p1, p2); // ring array
            p1 = p2;
        }
        ++p2;
    }
    SDL_ASSERT(!ring_n || (p1 == _end));
    return ring_n;
}

inline bool geo_base_polygon::ring_empty() const {
    return 0 == ring_num();
}

struct geo_polygon : geo_base_polygon { // = 26 bytes

    static constexpr spatial_type this_type = spatial_type::polygon;
};

struct geo_multipolygon : geo_base_polygon { // = 26 bytes

    static constexpr spatial_type this_type = spatial_type::multipolygon;
};

//------------------------------------------------------------------------

struct geo_linesegment_meta;
struct geo_linesegment_info;

struct geo_linesegment { // = 38 bytes

    using meta = geo_linesegment_meta;
    using info = geo_linesegment_info;

    static constexpr spatial_type this_type = spatial_type::linesegment;

    struct data_type {
        geo_head        head;       // 0x00 : 6 bytes
        spatial_point   points[2];  // 0x06 : 32 bytes
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    static constexpr size_t size() { 
        return 2;
    }
    spatial_point const * begin() const {
        return data.points;
    }
    spatial_point const * end() const {
        return data.points + size();
    }
    spatial_point const & operator[](size_t i) const {
        SDL_ASSERT(i < size());
        return data.points[i];
    }
    template<size_t i>
    spatial_point const & get() const {
        static_assert(i < size(), "");
        return data.points[i];
    }
    static constexpr size_t data_mem_size() {
        return sizeof(data_type);
    }
    bool contains(spatial_point const & p) const {
        return (data.points[0] == p)
            || (data.points[1] == p);
    }
};

#pragma pack(pop)

//------------------------------------------------------------------------

inline spatial_point const *
geo_tail::begin(geo_pointarray const & obj, size_t const subobj) const {
    SDL_ASSERT(subobj < size());
    if (subobj) {
        return obj.begin() + (*this)[subobj - 1];
    }
    return obj.begin();
}

inline spatial_point const *
geo_tail::end(geo_pointarray const & obj, size_t const subobj) const {
    SDL_ASSERT(subobj < size());
    if (subobj + 1 < size()) {
        return obj.begin() + (*this)[subobj];
    }
    return obj.end();
}

template<size_t const subobj> 
inline spatial_point const *
geo_tail::begin(geo_pointarray const & obj) const {
    SDL_ASSERT(subobj < size());
    if (subobj) {
        return obj.begin() + (*this)[subobj - 1];
    }
    return obj.begin();
}

template<size_t const subobj> 
inline spatial_point const *
geo_tail::end(geo_pointarray const & obj) const {
    SDL_ASSERT(subobj < size());
    if (subobj + 1 < size()) {
        return obj.begin() + get_num<subobj>();
    }
    return obj.end();
}

//------------------------------------------------------------------------

} // db
} // sdl

#endif // __SDL_SYSTEM_GEOGRAPHY_H__