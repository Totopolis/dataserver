// geography.cpp
//
#include "common/common.h"
#include "geography.h"
#include "system/page_info.h"

namespace sdl { namespace db {

geo_mem::geo_mem(data_type && m): m_data(std::move(m)) 
{
    init_geography();
    m_type = init_type();
    SDL_ASSERT(m_type != spatial_type::null);
}

void geo_mem::swap(geo_mem & v)
{
    m_data.swap(v.m_data);
    m_buf.swap(v.m_buf);
    std::swap(m_type, v.m_type);
    std::swap(m_geography, v.m_geography);
}

void geo_mem::init_geography()
{
    SDL_ASSERT(!m_geography && !m_buf);
    if (mem_size(m_data) > sizeof(geo_data)) {
        if (m_data.size() == 1) {
            m_geography = reinterpret_cast<geo_data const *>(m_data[0].first);
        }
        else {
            reset_new(m_buf, make_vector(m_data));
            m_geography = reinterpret_cast<geo_data const *>(m_buf->data());
        }
    }
    else {
        throw_error<geo_mem_error>("bad geography");
    }
    SDL_ASSERT(m_geography->data.SRID == 4326);
}

spatial_type geo_mem::init_type()
{
    static_assert(sizeof(geo_data) < sizeof(geo_point), "");
    static_assert(sizeof(geo_point) < sizeof(geo_multipolygon), "");
    static_assert(sizeof(geo_pointarray) < sizeof(geo_linesegment), "");
    static_assert(sizeof(geo_multipolygon) == sizeof(geo_pointarray), "");
    static_assert(sizeof(geo_linestring) == sizeof(geo_pointarray), "");

    geo_data const * const data = m_geography;
    size_t const data_size = mem_size(m_data);

    if (data_size == sizeof(geo_point)) { // 22 bytes
        if (data->data.tag == spatial_tag::t_point) {
            return spatial_type::point;
        }
        SDL_ASSERT(0);
        return spatial_type::null;
    }
    if (data_size == sizeof(geo_linesegment)) { // 38 bytes
        if (data->data.tag == spatial_tag::t_linesegment) {
            return spatial_type::linesegment;
        }
        SDL_ASSERT(0);
        return spatial_type::null;
    }
    if (data_size >= sizeof(geo_pointarray)) { // 26 bytes
        if (data->data.tag == spatial_tag::t_linestring) {
            SDL_ASSERT(!reinterpret_cast<const geo_linestring *>(data)->tail(data_size));
            return spatial_type::linestring;
        }
        if (data->data.tag == spatial_tag::t_multipolygon) {
            geo_base_polygon const * const pp = reinterpret_cast<const geo_base_polygon *>(data);
            geo_tail const * const tail = pp->tail(data_size);
            if (tail) {
                if (tail->size() > 1) {                    
                    SDL_ASSERT(tail->data.reserved.num == 0);
                    SDL_ASSERT(tail->data.numobj.num > 1);
                    if (tail->data.numobj.tag == 1) {
                        SDL_ASSERT(tail->data.reserved.tag == 1); 
                        return spatial_type::multilinestring;
                    }
                    else {
                        SDL_ASSERT((tail->data.reserved.tag == 0) || (tail->data.reserved.tag == 2)); 
                        SDL_ASSERT(tail->data.numobj.tag == 2);
                        SDL_ASSERT(!pp->ring_empty());
                        return spatial_type::multipolygon; // or polygon with interior rings
                    }
                }
                else {
                    SDL_ASSERT(tail->data.reserved.num == 0);
                    SDL_ASSERT(tail->data.reserved.tag == 1);
                    SDL_ASSERT(tail->data.numobj.num == 1);
                    if (tail->data.numobj.tag == 1) {
                        return spatial_type::linestring;
                    }
                    else {
                        SDL_ASSERT(tail->data.numobj.tag == 2);
                        SDL_ASSERT(pp->ring_num() == 1);
                        return spatial_type::polygon;
                    }
                }
            }
            SDL_ASSERT(0); // to be tested
            return spatial_type::linestring;
        }
    }
    SDL_ASSERT(0);
    return spatial_type::null;
}

std::string geo_mem::STAsText() const {
    if (!is_null()) {
        return to_string::type(*this);
    }
    SDL_ASSERT(0);
    return{};
}

bool geo_mem::STContains(spatial_point const & p) const
{
    switch (m_type) {
    case spatial_type::point:           return cast_point()->STContains(p);
//  case spatial_type::linestring:      return false; //cast_linestring()->STContains(p);
    case spatial_type::polygon:         return cast_polygon()->STContains(p);
//  case spatial_type::linesegment:     return false; //cast_linesegment()->STContains(p);
//  case spatial_type::multilinestring: return false; //cast_multilinestring()->STContains(p);
    case spatial_type::multipolygon:    return cast_multipolygon()->STContains(p);
    default:
        return false;
    }
}

geo_tail const * geo_mem::get_tail() const
{
    if (m_type == spatial_type::multipolygon) {
        return cast_multipolygon()->tail(mem_size(data()));
    }
    if (m_type == spatial_type::multilinestring) {
        return cast_multilinestring()->tail(mem_size(data()));
    }
    return nullptr;
}

orientation geo_mem::ring_orient(size_t const subobj) const
{
    SDL_ASSERT(m_type == spatial_type::multipolygon);
    if (subobj) {
        //point_access const p = get_subobj(subobj);
        //return geo_mem_::ring_orient(p.begin(), p.end());
    }
    return orientation::exterior;
}

//------------------------------------------------------------------------

spatial_point const *
geo_tail::begin(geo_pointarray const & obj, size_t const subobj) const
{
    SDL_ASSERT(subobj < size());
    if (subobj) {
        return obj.begin() + (*this)[subobj - 1];
    }
    return obj.begin();
}

spatial_point const *
geo_tail::end(geo_pointarray const & obj, size_t const subobj) const
{
    SDL_ASSERT(subobj < size());
    if (subobj + 1 < size()) {
        return obj.begin() + (*this)[subobj];
    }
    return obj.end();
}

} // db
} // sdl

