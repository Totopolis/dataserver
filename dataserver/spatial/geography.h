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

struct geo_tail { // 15 bytes

    struct num_type {       // 5 bytes
        uint32 num;         // value
        uint8 tag;          // byteorder ?
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
    num_type const & operator[](size_t i) const {
        SDL_ASSERT(i < this->size());
        return data.points[i];
    }
    num_type const * begin() const {
        return data.points;
    }
    num_type const * end() const {
        return data.points + this->size();
    }
    size_t data_mem_size() const {
        return sizeof(data_type) + sizeof(num_type) * size() - sizeof(data.points);
    }
};

//------------------------------------------------------------------------

struct geo_point_meta;
struct geo_point_info;

struct geo_point { // 22 bytes
    
    using meta = geo_point_meta;
    using info = geo_point_info;

    static const spatial_type this_type = spatial_type::point;

    struct data_type {
        geo_head      head;     // 0x00 : 6 bytes
        spatial_point point;    // 0x06 : 16 bytes
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
    static constexpr size_t data_mem_size() {
        return sizeof(data_type);
    }
    bool STContains(spatial_point const & p) const {
        return p == data.point;
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
        if ((data_size - data_mem_size()) >= sizeof(geo_tail)) {
            return reinterpret_cast<geo_tail const *>(this->end());
        }
        return nullptr;
    }
};

//------------------------------------------------------------------------

struct geo_linestring : geo_pointarray { // = 26 bytes

    static const spatial_type this_type = spatial_type::linestring;

    bool STContains(spatial_point const &) const {
        return false; //FIXME: not implemented
    }
};

struct geo_multilinestring : geo_pointarray { // = 26 bytes

    static const spatial_type this_type = spatial_type::multilinestring;

    bool STContains(spatial_point const &) const {
        return false; //FIXME: not implemented
    }
};

//------------------------------------------------------------------------

struct geo_base_multipolygon : geo_pointarray { // = 26 bytes

    static const spatial_type this_type = spatial_type::polygon;

    bool ring_empty() const;
    size_t ring_num() const;

    template<class fun_type>
    size_t for_ring(fun_type fun) const; //FIXME: https://en.wikipedia.org/wiki/Curve_orientation

    bool STContains(spatial_point const &) const;
};

template<class fun_type>
size_t geo_base_multipolygon::for_ring(fun_type fun) const
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

inline bool geo_base_multipolygon::ring_empty() const {
    return 0 == ring_num();
}

//------------------------------------------------------------------------

struct geo_polygon : geo_base_multipolygon { // = 26 bytes

    static const spatial_type this_type = spatial_type::polygon;
};

struct geo_multipolygon : geo_base_multipolygon { // = 26 bytes

    static const spatial_type this_type = spatial_type::multipolygon;
};

//------------------------------------------------------------------------

struct geo_linesegment_meta;
struct geo_linesegment_info;

struct geo_linesegment { // = 38 bytes

    using meta = geo_linesegment_meta;
    using info = geo_linesegment_info;

    static const spatial_type this_type = spatial_type::linesegment;

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
    static constexpr size_t data_mem_size() {
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
public:
    explicit geo_mem(data_type && m);
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
    std::string STAsText() const;
    bool STContains(spatial_point const &) const;

    template<class T> T const * cast_t() const && = delete;
    template<class T> T const * cast_t() const & {        
        SDL_ASSERT(T::this_type == m_type);    
        T const * const obj = reinterpret_cast<T const *>(this->geography());
        SDL_ASSERT(size() >= obj->data_mem_size());
        return obj;
    }
    geo_point const * cast_point() const && = delete;
    geo_polygon const * cast_polygon() const && = delete;    
    geo_multipolygon const * cast_multipolygon() const && = delete;
    geo_linesegment const * cast_linesegment() const && = delete;
    geo_linestring const * cast_linestring() const && = delete;    
    geo_multilinestring const * cast_multilinestring() const && = delete;    
    geo_point const * cast_point() const &                      { return cast_t<geo_point>(); }
    geo_polygon const * cast_polygon() const &                  { return cast_t<geo_polygon>(); }
    geo_multipolygon const * cast_multipolygon() const &        { return cast_t<geo_multipolygon>(); }
    geo_linesegment const * cast_linesegment() const &          { return cast_t<geo_linesegment>(); }
    geo_linestring const * cast_linestring() const &            { return cast_t<geo_linestring>(); }
    geo_multilinestring const * cast_multilinestring() const &  { return cast_t<geo_multilinestring>(); }  
private:
    spatial_type get_type(vector_mem_range_t const &);
    const char * geography() const;
    geo_mem(const geo_mem&) = delete;
    const geo_mem& operator = (const geo_mem&) = delete;
    void swap(geo_mem & src);
private:
    using buf_type = std::vector<char>;
    spatial_type m_type = spatial_type::null;
    mutable std::unique_ptr<buf_type> m_buf;
    mutable const char * m_geography = nullptr;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_GEOGRAPHY_H__