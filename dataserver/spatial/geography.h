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

struct geo_pointarray;
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
    size_t operator[](size_t i) const {
        SDL_ASSERT(i < this->size());
        return data.points[i].num;
    }
    size_t data_mem_size() const {
        return sizeof(data_type) + sizeof(num_type) * size() - sizeof(data.points);
    }
    spatial_point const * begin(geo_pointarray const &, size_t const subobj) const;
    spatial_point const * end(geo_pointarray const &, size_t const subobj) const;
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

struct geo_base_polygon : geo_pointarray { // = 26 bytes

    static const spatial_type this_type = spatial_type::polygon;

    bool ring_empty() const;
    size_t ring_num() const;

    template<class fun_type>
    size_t for_ring(fun_type fun) const; //FIXME: https://en.wikipedia.org/wiki/Curve_orientation

    bool STContains(spatial_point const &) const;
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

    static const spatial_type this_type = spatial_type::polygon;
};

struct geo_multipolygon : geo_base_polygon { // = 26 bytes

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

class geo_mem : noncopyable { // movable
    class point_access {
        using obj_type = geo_pointarray;
        obj_type const * const obj;
        geo_tail const * const tail;
        size_t const subobj;
    public:
        point_access(obj_type const * p, geo_tail const * t, size_t const s)
            : obj(p), tail(t), subobj(s) {
            SDL_ASSERT(obj && tail && (subobj < tail->size()));
        }
        spatial_point const * begin() const {
            return tail->begin(*obj, subobj);
        }
        spatial_point const * end() const {
            return tail->end(*obj, subobj);
        }
        size_t size() const {
            SDL_ASSERT(begin() < end());
            return end() - begin();
        }
    };
public:
    using data_type = vector_mem_range_t;
    geo_mem(){}
    explicit geo_mem(data_type && m);
    geo_mem(geo_mem && v): m_type(spatial_type::null) {
        this->swap(v);
    }
    const geo_mem & operator=(geo_mem && v) {
        this->swap(v);
        return *this;
    }
    bool is_null() const {
        return m_type == spatial_type::null;
    }
    explicit operator bool() const {
        return !is_null();
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
private:
    template<class T> T const * cast_t() const && = delete;
    template<class T> T const * cast_t() const & {        
        SDL_ASSERT(T::this_type == m_type);    
        T const * const obj = reinterpret_cast<T const *>(this->geography());
        SDL_ASSERT(size() >= obj->data_mem_size());
        return obj;
    }
    geo_pointarray const * cast_pointarray() const { // for get_subobj
        SDL_ASSERT((m_type == spatial_type::multipolygon) || 
                   (m_type == spatial_type::multilinestring));
        geo_pointarray const * const obj = reinterpret_cast<geo_pointarray const *>(this->geography());
        SDL_ASSERT(size() >= obj->data_mem_size());
        return obj;
    }
public:
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
    
    size_t numobj() const; // if multipolygon or multilinestring then numobj > 1 else numobj = 0 
    point_access get_subobj(size_t subobj) const;
private:
    spatial_type init_type();
    void init_geography();
    const char * geography() const;
    geo_tail const * get_tail() const;
    void swap(geo_mem &);
private:
    using buf_type = std::vector<char>;
    spatial_type m_type = spatial_type::null;
    const char * m_geography = nullptr;
    data_type m_data;
    std::unique_ptr<buf_type> m_buf;
};

inline size_t geo_mem::numobj() const {
    geo_tail const * const tail = get_tail();
    return tail ? tail->size() : 0;
}

inline geo_mem::point_access
geo_mem::get_subobj(size_t const subobj) const {
    SDL_ASSERT(subobj < numobj());
    return point_access(cast_pointarray(), get_tail(), subobj);
}


using geography_t = vector_mem_range_t; //FIXME: replace by geo_mem ?

} // db
} // sdl

#endif // __SDL_SYSTEM_GEOGRAPHY_H__