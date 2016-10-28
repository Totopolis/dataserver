// geography.cpp
//
#include "common/common.h"
#include "geography.h"
#include "math_util.h"
#include "transform.h"
#include "system/page_info.h"

namespace sdl { namespace db { namespace {

    //vector_mem_range_t => geo_mem_range
}

geo_mem::geo_mem(data_type && m): m_data(std::move(m)) {
    SDL_ASSERT(mem_size(m_data) > sizeof(geo_data));
    init_geography();
    m_type = init_type();
    SDL_ASSERT(m_type != spatial_type::null);
    init_ring_orient();
    SDL_ASSERT_DEBUG_2(STGeometryType() != geometry_types::Unknown);
}

geo_mem::point_access
geo_mem::get_subobj(size_t const subobj) const {
    SDL_ASSERT(subobj < numobj());
    if (geo_tail const * const tail = get_tail()) {
        geo_pointarray const * const p = cast_pointarray();
        return point_access(
            tail->begin(*p, subobj),
            tail->end(*p, subobj), 
            m_buf);
    }
    SDL_ASSERT(0);
    return{};
}

geo_mem::point_access
geo_mem::get_exterior() const { // get_subobj(0)
    SDL_ASSERT(numobj());
    if (geo_tail const * const tail = get_tail()) {
        geo_pointarray const * const p = cast_pointarray();
        return point_access(tail->begin<0>(*p), tail->end<0>(*p), m_buf);
    }
    SDL_ASSERT(0);
    return{};
}

const geo_mem &
geo_mem::operator=(geo_mem && v) noexcept {
    m_data = std::move(v.m_data);
    m_buf = std::move(v.m_buf);
    m_type = v.m_type; v.m_type = spatial_type::null; // move m_type
    m_geography = v.m_geography; v.m_geography = nullptr; // move m_geography
    m_ring_orient = std::move(v.m_ring_orient);
    SDL_ASSERT(!v.m_buf);
    SDL_ASSERT(!v.m_ring_orient);
    SDL_ASSERT((m_geography != nullptr) == (m_type != spatial_type::null));
    SDL_ASSERT((m_data.size() > 1) == (m_buf.get() != nullptr));
    return *this;
}

void geo_mem::swap(geo_mem & v) noexcept {
    static_check_is_nothrow_move_assignable(m_data);
    static_check_is_nothrow_move_assignable(m_buf);
    static_check_is_nothrow_move_assignable(m_type);
    static_check_is_nothrow_move_assignable(m_geography);
    static_check_is_nothrow_move_assignable(m_ring_orient);
    m_data.swap(v.m_data);
    m_buf.swap(v.m_buf);
    std::swap(m_type, v.m_type);
    std::swap(m_geography, v.m_geography);
    std::swap(m_ring_orient, v.m_ring_orient);
}

void geo_mem::init_geography()
{
    SDL_ASSERT(!m_geography && !m_buf);
    if (mem_size(m_data) > sizeof(geo_data)) {
        if (m_data.size() == 1) {
            m_geography = reinterpret_cast<geo_data const *>(m_data[0].first);
        }
        else {
            reset_new(m_buf, make_vector(m_data)); //FIXME: will be replaced : iterate memory without copy
            m_geography = reinterpret_cast<geo_data const *>(m_buf->data());
        }
    }
    else {
        throw_error<sdl_exception_t<geo_mem>>("bad geography");
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
                        //FIXME: GEOMETRYCOLLECTION
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
            return spatial_type::null;
        }
    }
    SDL_ASSERT(0);
    return spatial_type::null;
}

std::string geo_mem::STAsText() const
{
    if (is_null()) {
        return {}; 
    }
    return to_string::type(*this);
}

bool geo_mem::STContains(spatial_point const & p) const
{
    if (is_null()) {
        return false; 
    }
    switch (m_type) {
    case spatial_type::point:
        return cast_point()->is_equal(p);
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
        SDL_WARNING(!"not implemented");
        return false;
    }
}

bool geo_mem::STIntersects(spatial_rect const & rc, intersect_type const flag) const
{
    if (is_null()) {
        return false;
    }
    switch (m_type) {
    case spatial_type::point:
        return transform::STIntersects(rc, cast_point()->data.point);
    case spatial_type::linestring:
        return transform::STIntersects(rc, *cast_linestring(), intersect_type::linestring);
    case spatial_type::polygon:
        return transform::STIntersects(rc, *cast_polygon(), flag);
    case spatial_type::linesegment:
        return transform::STIntersects(rc, *cast_linesegment(), intersect_type::linestring);
    case spatial_type::multilinestring:
        for (size_t i = 0, num = numobj(); i < num; ++i) {
            if (transform::STIntersects(rc, get_subobj(i), intersect_type::linestring)) {
                return true;
            }
        }
        break;
    case spatial_type::multipolygon:
        if (flag == intersect_type::linestring) {
            for (size_t i = 0, num = numobj(); i < num; ++i) {
                if (transform::STIntersects(rc, get_subobj(i), intersect_type::linestring)) {
                    return true;
                }
            }
        }
        else {
            SDL_ASSERT(flag == intersect_type::polygon);
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
    return false;
}

bool geo_mem::STIntersects(spatial_rect const & rc) const
{
    intersect_type const flag =
        ((m_type == spatial_type::polygon) ||
        (m_type == spatial_type::multipolygon)) ? intersect_type::polygon : intersect_type::linestring;
    return STIntersects(rc, flag);    
}

Meters geo_mem::STDistance(spatial_point const & where) const
{
    if (is_null()) {
        return 0; 
    }
    if (const size_t num = numobj()) { // multilinestring | multipolygon
        SDL_ASSERT(num > 1);
        if (m_type == spatial_type::multipolygon) {
            Meters min_dist = transform::STDistance(get_exterior(), where, intersect_type::polygon);
            auto const & orient = ring_orient();
            for (size_t i = 1; i < num; ++i) {
                if (orient[i] == orientation::exterior) {
                    const Meters d = transform::STDistance(get_subobj(i), where, intersect_type::polygon);
                    if (d.value() < min_dist.value()) {
                        min_dist = d;
                    }
                }
            }
            return min_dist;
        }
        else {
            SDL_ASSERT(m_type == spatial_type::multilinestring);
            Meters min_dist = transform::STDistance(get_exterior(), where, intersect_type::linestring);
            for (size_t i = 1; i < num; ++i) {
                const Meters d = transform::STDistance(get_subobj(i), where, intersect_type::linestring);
                if (d.value() < min_dist.value()) {
                    min_dist = d;
                }
            }
            return min_dist;
        }
    }
    else {
        switch (m_type) {
        case spatial_type::point:  
            return transform::STDistance(cast_point()->data.point, where);
        case spatial_type::linestring:
            return transform::STDistance(*cast_linestring(), where, intersect_type::linestring);
        case spatial_type::polygon: 
            return transform::STDistance(*cast_polygon(), where, intersect_type::polygon);
        case spatial_type::linesegment:
            return transform::STDistance(*cast_linesegment(), where, intersect_type::linestring);
        default:
            SDL_ASSERT(0); 
            return 0;
        }
    }
}

Meters geo_mem::STDistance(geo_mem const & src) const
{
    if (is_null() || src.is_null()) {
        return 0;
    }
    if (m_geography == src.m_geography) {
        SDL_ASSERT(algo::is_same(m_data, src.m_data));
        return 0;
    }
    if (algo::is_same(m_data, src.m_data)) {
        return 0;
    }
    if (src.type() == spatial_type::point) {
        return STDistance(src.cast_point()->data.point);
    }
    SDL_ASSERT(!"STDistance"); // not implemented
    return 0;
}

Meters geo_mem::STLength() const
{
    if (is_null()) {
        return 0; 
    }
    switch (m_type) {
    case spatial_type::point: 
        return 0;
    case spatial_type::linestring:
        return transform::STLength(*cast_linestring());
    case spatial_type::polygon: 
        return transform::STLength(*cast_polygon());
    case spatial_type::linesegment:
        return transform::STLength(*cast_linesegment());
    case spatial_type::multilinestring:
    case spatial_type::multipolygon:
        if (const size_t num = numobj()) { // multilinestring | multipolygon
            SDL_ASSERT(num > 1);
            Meters length = transform::STLength(get_exterior());
            for (size_t i = 1; i < num; ++i) {
                length += transform::STLength(get_subobj(i));
            }
            return length;
        }
        SDL_ASSERT(0); 
        return 0;
    default:
        SDL_ASSERT(0); 
        return 0;
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

namespace {
    template<class T>
    orientation get_orientation(T const & exterior, T const & subobj) {
        bool point_on_vertix = false;
        for (auto const & p : subobj) {
            if (math_util::point_in_polygon(exterior.begin(), exterior.end(), p, point_on_vertix)) {
                if (!point_on_vertix) {
                    return orientation::interior;
                }
            }
            else {
                break;
            }
        }
        return orientation::exterior;
    }
}

void geo_mem::init_ring_orient()
{
    SDL_ASSERT(!m_ring_orient); // init once
    if (geo_tail const * const tail = get_tail_multipolygon()) {
        const size_t size = tail->size();
        reset_new(m_ring_orient, size, orientation::exterior);
        vec_orientation & result = *m_ring_orient;
        point_access exterior = get_exterior();
        for (size_t i = 1; i < size; ++i) {
            point_access next = get_subobj(i);
            SDL_ASSERT(is_exterior(result[0]));
            bool exterior_inside_interior = false;
            for (size_t j = i - 1; is_interior(result[j]); --j) {
                if (is_interior(get_orientation(get_subobj(j), next))) {
                    SDL_ASSERT(result[i] == orientation::exterior);
                    exterior = next;
                    exterior_inside_interior = true;
                    break;
                }
            }
            if (exterior_inside_interior)
                continue;
            if (is_interior(get_orientation(exterior, next))) {
                result[i] = orientation::interior;
            }
            else {
                exterior = next;
            }
        }
        SDL_ASSERT(result.size() == numobj());
    }
}

geo_mem::vec_orientation const &
geo_mem::ring_orient() const 
{
    if (m_ring_orient) {
        return *m_ring_orient;
    }
    static const vec_orientation empty;
    return empty;
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

bool geo_mem::multiple_exterior() const
{
    size_t exterior = 0;
    for (auto p : ring_orient()) {
        if (is_exterior(p)) {
            if (exterior++) {
                return true;
            }
        }
    }
    return false;
}

geometry_types geo_mem::STGeometryType() const
{
    switch (m_type) {
    case spatial_type::point:           return geometry_types::Point;
    case spatial_type::linestring:      return geometry_types::LineString;
    case spatial_type::linesegment:     return geometry_types::LineString;
    case spatial_type::polygon:         return geometry_types::Polygon;
    case spatial_type::multilinestring: return geometry_types::MultiLineString;
    case spatial_type::multipolygon:
        return multiple_exterior() ? geometry_types::MultiPolygon : geometry_types::Polygon;
    default:
        SDL_ASSERT(m_type == spatial_type::null);
        break;
    }
    return geometry_types::Unknown;
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