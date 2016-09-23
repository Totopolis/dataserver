// geography.cpp
//
#include "common/common.h"
#include "geography.h"
#include "math_util.h"
#include "transform.h"
#include "system/page_info.h"

namespace sdl { namespace db {

geo_mem::geo_mem(data_type && m): m_data(std::move(m)) {
    init_geography();
    m_type = init_type();
    SDL_ASSERT(m_type != spatial_type::null);
}

const geo_mem &
geo_mem::operator=(geo_mem && v) {
    m_data = std::move(v.m_data);
    m_buf = std::move(v.m_buf);
    m_type = v.m_type; v.m_type = spatial_type::null; // move m_type
    m_geography = v.m_geography; v.m_geography = nullptr; // move m_geography
    SDL_ASSERT(!v.m_buf);
    SDL_ASSERT((m_geography != nullptr) == (m_type != spatial_type::null));
    SDL_ASSERT((m_data.size() > 1) == (m_buf.get() != nullptr));
    return *this;
}

void geo_mem::swap(geo_mem & v) {
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
    case spatial_type::point:
        return cast_point()->data.point == p;
    case spatial_type::polygon:
        return transform::STContains(*cast_polygon(), p);
    case spatial_type::multipolygon: {
            auto const & orient = ring_orient();
            for (size_t i = 0, num = numobj(); i < num; ++i) {
                if (orient[i] == orientation::exterior) {
                    if (transform::STContains(get_subobj(i), p)) {
                        return true;
                    }
                }
            }
            return false;
        }
    default:
        return false; // not implemented or is_null()
    }
}

bool geo_mem::STIntersects(spatial_rect const & rc) const
{
    if (!is_null()) {
        switch (m_type) {
        case spatial_type::point:
            return transform::STIntersects(rc, cast_point()->data.point);
        case spatial_type::linestring:
            return transform::STIntersects(rc, *cast_linestring(), intersect_type::linestring);
        case spatial_type::polygon:
            return transform::STIntersects(rc, *cast_polygon(), intersect_type::polygon);
        case spatial_type::linesegment:
            return transform::STIntersects(rc, *cast_linesegment(), intersect_type::linestring);
        case spatial_type::multilinestring:
            for (size_t i = 0, num = numobj(); i < num; ++i) {
                if (transform::STIntersects(rc, get_subobj(i), intersect_type::linestring)) {
                    return true;
                }
            }
            break;
        case spatial_type::multipolygon: {
                auto const & orient = ring_orient();
                for (size_t i = 0, num = numobj(); i < num; ++i) {
                    if (orient[i] == orientation::exterior) {
                        if (transform::STIntersects(rc, get_subobj(i), intersect_type::polygon)) {
                            return true;
                        }
                    }
                }
            }
            break;
        default:
            SDL_ASSERT(0);
            break;
        }
    }
    return false;
}

Meters geo_mem::STDistance(spatial_point const & where, spatial_rect const * const bbox) const
{
    if (const size_t num = numobj()) { // multilinestring | multipolygon
        SDL_ASSERT(num > 1);
        Meters min_dist = transform::STDistance(get_subobj(0), where, bbox);
        for (size_t i = 1; i < num; ++i) {
            const Meters d = transform::STDistance(get_subobj(i), where, bbox);
            if (d.value() < min_dist.value()) {
                min_dist = d;
            }
        }
        return min_dist;
    }
    else {
        switch (m_type) {
        case spatial_type::point:  
            return transform::STDistance(cast_point()->data.point, where);
        case spatial_type::linestring:
            return transform::STDistance(*cast_linestring(), where, bbox);
        case spatial_type::polygon: 
            return transform::STDistance(*cast_polygon(), where, bbox);
        case spatial_type::linesegment:
            return transform::STDistance(*cast_linesegment(), where, bbox);
        default:
            SDL_ASSERT(0); 
            return 0;
        }
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

geo_tail const * geo_mem::get_tail_multipolygon() const
{
    if (m_type == spatial_type::multipolygon) {
        return cast_multipolygon()->tail(mem_size(data()));
    }
    return nullptr;
}

geo_mem::vec_orientation
geo_mem::ring_orient() const
{
    if (geo_tail const * const tail = get_tail_multipolygon()) {
        const size_t size = tail->size();
        vec_orientation result(size, orientation::exterior);
        point_access exterior = get_subobj(0);
        bool point_on_vertix = false;
        for (size_t i = 1; i < size; ++i) {
            point_access next = get_subobj(i);
            for (auto const & p : next) {
                if (math_util::point_in_polygon(exterior.begin(), exterior.end(), p, point_on_vertix)) {
                    if (!point_on_vertix) {
                        result[i] = orientation::interior;
                        break;
                    }
                }
                else {
                    exterior = next;
                    break;
                }
            }
        }
        SDL_ASSERT(result.size() == numobj());
        return result;
    }
    return {};
}

geo_mem::vec_winding
geo_mem::ring_winding() const
{
    if (geo_tail const * const tail = get_tail_multipolygon()) {
        const size_t size = tail->size();
        vec_winding result(size);
        for (size_t i = 0; i < size; ++i) {
            result[i] = math_util::ring_winding(get_subobj(i));
        }
        return result;
    }
    return {};
}

//------------------------------------------------------------------------

} // db
} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    if (1) {
                        using vec = geo_mem::vec_orientation;
                        //SDL_TRACE(sizeof(vec));
                        vec t1(4, orientation::interior);
                        vec t2(20, orientation::interior);
                        SDL_ASSERT(t1.use_buf());
                        SDL_ASSERT(!t2.use_buf());
                        vec m1(std::move(t1));
                        vec m2(std::move(t2));
                        vec m11, m22;
                        m11 = std::move(m1);
                        m22 = std::move(m2);
                        SDL_ASSERT(m11.size() == 4);
                        SDL_ASSERT(m22.size() == 20);
                        m22 = std::move(m11);
                        SDL_ASSERT(m22.size() == 4);
                        m22.clear();
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG