// geography.h
//
#pragma once
#ifndef __SDL_SPATIAL_GEOGRAPHY_H__
#define __SDL_SPATIAL_GEOGRAPHY_H__

#include "dataserver/common/array.h"
#include "dataserver/spatial/geo_data.h"
#include "dataserver/spatial/transform.h"

namespace sdl { namespace db {

struct geo_closest_point_t { // POD
    track_closest_point_t base;
    size_t subobj; // index of subobject
};

class geo_mem : noncopyable { // movable
    using buf_type = std::vector<char>;
    using shared_buf = std::shared_ptr<buf_type>;
public:
    class point_access {
        spatial_point const * m_begin;
        spatial_point const * m_end;
        shared_buf m_buf; // reference to temporal memory in use
    private:
        friend class geo_mem;
        point_access(spatial_point const * const begin,
                     spatial_point const * const end,
                     shared_buf const & buf) noexcept
            : m_begin(begin), m_end(end), m_buf(buf)
        {
            SDL_ASSERT(m_begin && m_end && !empty());
        }
        template<class T>
        point_access(T const & obj, shared_buf const & buf)
            : m_begin(obj.begin()), m_end(obj.end()), m_buf(buf)
        {
            SDL_ASSERT(m_begin && m_end && !empty());
        }
    public:
        point_access() noexcept: m_begin(nullptr), m_end(nullptr) {}
        spatial_point const * begin() const noexcept {
            return m_begin;
        }
        spatial_point const * end() const noexcept {
            return m_end;
        }
        bool empty() const noexcept {
            SDL_ASSERT(m_begin <= m_end);
            return m_begin == m_end;
        }
        explicit operator bool() const noexcept {
            return !empty();
        }
        size_t size() const noexcept {
            SDL_ASSERT(m_begin <= m_end);
            return m_end - m_begin;
        }
        spatial_point const & operator[](size_t const i) const noexcept {
            SDL_ASSERT(i < this->size());
            return *(begin() + i);
        }
        spatial_point const & front() const noexcept {
            SDL_ASSERT(!empty());
            return *begin();
        }
        spatial_point const & back() const noexcept {
            SDL_ASSERT(!empty());
            return *(end() - 1);
        }
        template<size_t i>
        spatial_point const & prev_back() const noexcept {
            SDL_ASSERT(this->size() > i);
            return *(end() - 1 - i);
        }
        template<size_t i>
        spatial_point const & next_front() const noexcept {
            SDL_ASSERT(this->size() > i);
            return *(begin() + i);
        }
#if SDL_DEBUG
        shared_buf const & use_buf() const {
            return m_buf;
        }
#endif
    };
public:
    using data_type = vector_mem_range_t;
    geo_mem() noexcept {}
    geo_mem(data_type && m); // allow implicit conversion
    geo_mem(geo_mem && v) noexcept {
        (*this) = std::move(v);
    }
    ~geo_mem();
    void swap(geo_mem &) noexcept;
    geo_mem & operator=(geo_mem &&) noexcept;
    spatial_type type() const noexcept {
        return pdata ? pdata->m_type : spatial_type::null;
    }
    bool is_null() const noexcept {
        return type() == spatial_type::null;
    }
    explicit operator bool() const noexcept {
        return !is_null();
    }
    data_type const & data() const noexcept {
        return pdata->m_data;
    }
    size_t size() const noexcept {
        return mem_size(data());
    }
    geometry_types STGeometryType() const;
    std::string STAsText() const;
    std::string substr_STAsText(size_t pos, size_t count, bool) const;
    bool STContains(spatial_point const &) const;
    bool STContains(geo_mem const &) const;
    bool STIntersects(spatial_rect const &) const;
    bool STIntersects(spatial_rect const &, intersect_flag) const;
    Meters STDistance(spatial_point const &) const;
    Meters STDistance(geo_mem const &) const;
    Meters STLength() const;
    geo_closest_point_t STClosestpoint(spatial_point const &) const;
    spatial_rect envelope() const;
private:
    bool multipolygon_STContains(spatial_point const &, orientation) const;
    template<class T> T const * cast_t() const && = delete;
    template<class T> T const * cast_t() const & noexcept {        
        SDL_ASSERT(T::this_type == type());    
        T const * const obj = reinterpret_cast<T const *>(pdata->m_geography);
        SDL_ASSERT(size() >= obj->data_mem_size());
        return obj;
    }
    geo_pointarray const * cast_pointarray() const noexcept { // for get_subobj
        SDL_ASSERT((type() == spatial_type::multipolygon) || 
                   (type() == spatial_type::multilinestring));
        geo_pointarray const * const obj = reinterpret_cast<geo_pointarray const *>(pdata->m_geography);
        SDL_ASSERT(size() >= obj->data_mem_size());
        return obj;
    }
    bool is_same(geo_mem const & src) const;
public:
    void cast_point() const && = delete;
    void cast_polygon() const && = delete;    
    void cast_multipolygon() const && = delete;
    void cast_linesegment() const && = delete;
    void cast_linestring() const && = delete;    
    void cast_multilinestring() const && = delete;    
    geo_point const * cast_point() const &                      { return cast_t<geo_point>(); }
    geo_polygon const * cast_polygon() const &                  { return cast_t<geo_polygon>(); }
    geo_multipolygon const * cast_multipolygon() const &        { return cast_t<geo_multipolygon>(); }
    geo_linesegment const * cast_linesegment() const &          { return cast_t<geo_linesegment>(); }
    geo_linestring const * cast_linestring() const &            { return cast_t<geo_linestring>(); }
    geo_multilinestring const * cast_multilinestring() const &  { return cast_t<geo_multilinestring>(); }  
    
    size_t numobj() const; // if multipolygon or multilinestring then numobj > 1 else numobj = 0 
    point_access get_subobj(size_t subobj) const;
    point_access get_exterior() const;

    using vec_orientation = vector_buf<orientation, 16>;
    using vec_winding = vector_buf<winding, 16>;
    vec_orientation const & ring_orient() const;
    vec_winding ring_winding() const;    
    bool multiple_exterior() const;
private:
    spatial_type init_type();
    void init_geography();
    geo_tail const * get_tail() const;
    geo_tail const * get_tail_multipolygon() const;
private:
    using unique_vec_orientation = std::unique_ptr<vec_orientation>;
    struct this_data : noncopyable {
        spatial_type m_type = spatial_type::null;
        geo_data const * m_geography = nullptr;
        data_type m_data;
        shared_buf m_buf;
        std::pair<unique_vec_orientation, bool> m_ring_orient; // init once
        this_data() = default;
        explicit this_data(data_type && m) noexcept: m_data(std::move(m)) {}
    }; 
    std::unique_ptr<this_data> pdata;
    shared_buf const & pdata_buf() const {
        SDL_ASSERT(pdata);
        return pdata->m_buf;
    }
    void init_ring_orient(unique_vec_orientation &) const;
};

inline size_t geo_mem::numobj() const {
    geo_tail const * const tail = get_tail();
    return tail ? tail->size() : 0;
}

inline geo_mem &
geo_mem::operator=(geo_mem && v) noexcept {
    pdata = std::move(v.pdata);
    return *this;
}

inline void geo_mem::swap(geo_mem & v) noexcept {
    static_check_is_nothrow_move_assignable(pdata);
    pdata.swap(v.pdata);
}

} // db
} // sdl

#endif // __SDL_SPATIAL_GEOGRAPHY_H__