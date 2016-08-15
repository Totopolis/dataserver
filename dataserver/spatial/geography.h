// geography.h
//
#pragma once
#ifndef __SDL_SYSTEM_GEOGRAPHY_H__
#define __SDL_SYSTEM_GEOGRAPHY_H__

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
        uint32  SRID;       // 0x00 : 4 bytes // E6100000 = 4326 (WGS84 — SRID 4326)
        uint16  tag;        // 0x04 : 2 bytes // = TYPEID
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    static spatial_type get_type(vector_mem_range_t const &);
};

using geo_head = geo_data::data_type;

//------------------------------------------------------------------------

struct geo_point_meta;
struct geo_point_info;

struct geo_point { // 22 bytes
    
    using meta = geo_point_meta;
    using info = geo_point_info;

    static const spatial_type this_type = spatial_type::point;
    static const uint16 TYPEID = (uint16)this_type; // 3073

    struct data_type {
        geo_head      head;     // 0x00 : 6 bytes
        spatial_point point;    // 0x06 : 16 bytes
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    static constexpr size_t mem_size() {
        return sizeof(data_type);
    }
    bool STContains(spatial_point const & p) const {
        return p == data.point;
    }
};

//------------------------------------------------------------------------

struct geo_multipolygon_meta;
struct geo_multipolygon_info;

struct geo_multipolygon { // = 26 bytes

    using meta = geo_multipolygon_meta;
    using info = geo_multipolygon_info;

    static const spatial_type this_type = spatial_type::multipolygon;
    static const uint16 TYPEID = (uint16)this_type; // 1025

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
    size_t mem_size() const {
        return sizeof(data_type)-sizeof(spatial_point)+sizeof(spatial_point)*size();
    }
    size_t ring_num() const;

    template<class fun_type>
    size_t for_ring(fun_type fun) const;

    bool STContains(spatial_point const &) const;
};

template<class fun_type>
size_t geo_multipolygon::for_ring(fun_type fun) const
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
    return ring_n;
}

//------------------------------------------------------------------------

struct geo_linestring_meta;
struct geo_linestring_info;

struct geo_linestring { // = 38 bytes, linesegment

    using meta = geo_linestring_meta;
    using info = geo_linestring_info;

    static const spatial_type this_type = spatial_type::linestring;
    static const uint16 TYPEID = (uint16)this_type; // 5121

    struct data_type {
        geo_head        head;       // 0x00 : 6 bytes
        spatial_point   first;      // 0x06 : 16 bytes
        spatial_point   second;     // 0x16 : 16 bytes
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    static size_t size() { 
        return 2;
    } 
    spatial_point const & operator[](size_t i) const {
        SDL_ASSERT(i < size());
        return (0 == i) ? data.first : data.second;
    }
    static constexpr size_t mem_size() {
        return sizeof(data_type);
    }
    bool STContains(spatial_point const &) const {
        return false; //FIXME: not implemented
    }
};

#pragma pack(pop)

//------------------------------------------------------------------------

using geography_t = vector_mem_range_t; //FIXME: replace by geo_mem ?

class geo_mem_base {
protected:
    using data_type = vector_mem_range_t;
    data_type m_data;
    geo_mem_base() = default;
    explicit geo_mem_base(data_type && m): m_data(std::move(m)){}
};

class geo_mem : geo_mem_base {
    using buf_type = std::vector<char>;
    spatial_type m_type;
    mutable std::unique_ptr<buf_type> m_buf;
    mutable const char * m_geography = nullptr;
    const char * geography() const;
    geo_mem(const geo_mem&) = delete;
    const geo_mem& operator = (const geo_mem&) = delete;
    void swap(geo_mem & src);
public:
    explicit geo_mem(data_type && m): geo_mem_base(std::move(m))
        , m_type(geo_data::get_type(m_data)) {
        SDL_ASSERT(size());
        SDL_ASSERT(m_type != spatial_type::null);
    }
    geo_mem(geo_mem && v): m_type(spatial_type::null) {
        this->swap(v);
        SDL_ASSERT(m_type != spatial_type::null);
    }
    const geo_mem & operator=(geo_mem && v) {
        this->swap(v);
        return *this;
    }
    bool is_valid() const {
        return m_type != spatial_type::null;
    }
    spatial_type type() const {
        return m_type;
    }
    data_type const & data() const {
        return m_data;
    }
    size_t size() const {
        return mem_size(m_data);
    }
    template<class T>
    T const * cast_t() const {
        SDL_ASSERT(T::this_type == m_type);        
        T const * const obj = reinterpret_cast<T const *>(geography());
        SDL_ASSERT(size() >= obj->mem_size());
        return obj;
    }
    geo_point const * cast_point() const {
        return cast_t<geo_point>();
    }
    geo_multipolygon const * cast_multipolygon() const {
        return cast_t<geo_multipolygon>();
    }
    geo_linestring const * cast_linestring() const {
        return cast_t<geo_linestring>();
    }
    std::string STAsText() const;
    bool STContains(spatial_point const &) const;

#if 0 // reserved
    operator geo_point const * () const {
        if (m_type == geo_point::this_type) {
            return cast_point();
        }
        return nullptr;
    }
    operator geo_multipolygon const * () const {
        if (m_type == geo_multipolygon::this_type) {
            return cast_multipolygon();
        }
        return nullptr;
    }
    operator geo_linestring const * () const {
        if (m_type == geo_linestring::this_type) {
            return cast_linestring();
        }
        return nullptr;
    }
#endif
};

} // db
} // sdl

#endif // __SDL_SYSTEM_GEOGRAPHY_H__