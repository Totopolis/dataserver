// geography.cpp
//
#include "dataserver/spatial/geography.h"
#include "dataserver/spatial/math_util.h"
#include "dataserver/system/page_info.h"

namespace sdl { namespace db {

geo_mem::~geo_mem() {}

geo_mem::geo_mem(data_type && m)
    : pdata(new this_data(std::move(m)))
{
    SDL_ASSERT(mem_size(data()) > sizeof(geo_data));
    init_geography();
    pdata->m_type = init_type();
    SDL_ASSERT(!is_null());
    SDL_ASSERT_DEBUG_2(STGeometryType() != geometry_types::Unknown);
}

geo_mem::point_access
geo_mem::get_subobj(size_t const subobj) const
{
    SDL_ASSERT(subobj < numobj());
    if (geo_tail const * const tail = get_tail()) {
        geo_pointarray const * const p = cast_pointarray();
        return point_access(
            tail->begin(*p, subobj),
            tail->end(*p, subobj), 
            pdata_buf());
    }
    SDL_ASSERT(0);
    return{};
}

geo_mem::point_access
geo_mem::get_exterior() const
{
    if (geo_tail const * const tail = get_tail()) {
        geo_pointarray const * const p = cast_pointarray();
        return point_access(tail->begin<0>(*p), tail->end<0>(*p), pdata_buf()); // get_subobj(0)
    }
    else {
        switch (type()) {
        case spatial_type::point:
            return point_access(*cast_point(), pdata_buf());
        case spatial_type::linestring:
            return point_access(*cast_linestring(), pdata_buf());
        case spatial_type::polygon:
            return point_access(*cast_polygon(), pdata_buf());
        case spatial_type::linesegment:
            return point_access(*cast_linesegment(), pdata_buf());
        default:
            SDL_ASSERT(0); 
            return {};
        }
    }
}

void geo_mem::init_geography()
{
    this_data & d = *pdata;
    SDL_ASSERT(!d.m_geography && !d.m_buf);
    if (mem_size(d.m_data) > sizeof(geo_data)) {
        if (d.m_data.size() == 1) {
            d.m_geography = reinterpret_cast<geo_data const *>(d.m_data[0].first);
        }
        else {
            reset_new(d.m_buf, mem_utils::make_vector(d.m_data)); //FIXME: vector_view ? without memory copy
            d.m_geography = reinterpret_cast<geo_data const *>(d.m_buf->data());
        }
    }
    else {
        throw_error<sdl_exception_t<geo_mem>>("bad geography");
    }
    SDL_ASSERT(d.m_geography->data.SRID == 4326);
}

bool geo_mem::is_same(geo_mem const & src) const
{
    SDL_ASSERT((pdata->m_geography != src.pdata->m_geography) || algo::is_same(pdata->m_data, src.pdata->m_data));
    return (pdata->m_geography == src.pdata->m_geography) || algo::is_same(pdata->m_data, src.pdata->m_data);
}

spatial_type geo_mem::init_type()
{
    static_assert(sizeof(geo_data) < sizeof(geo_point), "");
    static_assert(sizeof(geo_point) < sizeof(geo_multipolygon), "");
    static_assert(sizeof(geo_pointarray) < sizeof(geo_linesegment), "");
    static_assert(sizeof(geo_multipolygon) == sizeof(geo_pointarray), "");
    static_assert(sizeof(geo_linestring) == sizeof(geo_pointarray), "");

    geo_data const * const data = pdata->m_geography;
    size_t const data_size = mem_size(pdata->m_data);

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
                        SDL_ASSERT_DISABLED(pp->ring_num());
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
                        SDL_ASSERT_DISABLED(pp->ring_num() == 1);
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

std::string geo_mem::substr_STAsText(const size_t pos, const size_t count) const
{
    if (is_null()) {
        return {}; 
    }
    SDL_ASSERT((STGeometryType() == geometry_types::LineString)
            || (STGeometryType() == geometry_types::MultiLineString));
    return to_string::type_substr(*this, pos, count);
}

bool geo_mem::multipolygon_STContains(spatial_point const & p, const orientation flag) const
{
    SDL_ASSERT(type() == spatial_type::multipolygon);
    auto const & orient = ring_orient();
    for (size_t i = 0, num = numobj(); i < num; ++i) {
        if (orient[i] == flag) {
            if (transform_t::STContains(get_subobj(i), p)) {
                return true;
            }
        }
    }
    return false;
}

spatial_rect geo_mem::envelope() const
{
    if (is_null()) {
        return {};
    }
    switch (type()) {
    case spatial_type::point:
        return cast_point()->envelope();
    case spatial_type::polygon:
        return cast_polygon()->envelope();
    case spatial_type::multipolygon:
        return cast_multipolygon()->envelope();
    case spatial_type::linestring:
        return cast_linestring()->envelope();
    case spatial_type::linesegment:
        return cast_linesegment()->envelope();
    case spatial_type::multilinestring:
        return cast_multilinestring()->envelope();
    default:
        SDL_ASSERT(!"not implemented");
        return {};
    }
}

bool geo_mem::STContains(spatial_point const & p) const
{
    if (is_null()) {
        return false; 
    }
    switch (type()) {
    case spatial_type::point:
        return cast_point()->is_equal(p);
    case spatial_type::polygon:
        return transform_t::STContains(*cast_polygon(), p);
    case spatial_type::multipolygon: 
        if (multipolygon_STContains(p, orientation::exterior)) {
            return !multipolygon_STContains(p, orientation::interior);
        }
        return false;
    case spatial_type::linestring:
        return cast_linestring()->contains(p);
    case spatial_type::linesegment:
        return cast_linesegment()->contains(p);
    case spatial_type::multilinestring:
        return cast_multilinestring()->contains(p);
    default:
        SDL_ASSERT(!"not implemented");
        return false;
    }
}

bool geo_mem::STContains(geo_mem const & src) const
{
    if (is_null() || src.is_null()) {
        return false;
    }
    if (is_same(src)) {
        return true;
    }
    if (src.type() == spatial_type::point) {
        return STContains(src.cast_point()->data.point);
    }
    SDL_ASSERT(!"STContains"); // not implemented
    return false;
}

bool geo_mem::STIntersects(spatial_rect const & rc, intersect_flag const flag) const
{
    if (is_null()) {
        return false;
    }
    switch (type()) {
    case spatial_type::point:
        return transform::STIntersects(rc, cast_point()->data.point);
    case spatial_type::linestring:
        return transform_t::STIntersects<intersect_flag::linestring>(rc, *cast_linestring());
    case spatial_type::polygon:
        return transform_t::STIntersects(rc, *cast_polygon(), flag);
    case spatial_type::linesegment:
        return transform_t::STIntersects<intersect_flag::linestring>(rc, *cast_linesegment());
    case spatial_type::multilinestring:
        for (size_t i = 0, num = numobj(); i < num; ++i) {
            if (transform_t::STIntersects<intersect_flag::linestring>(rc, get_subobj(i))) {
                return true;
            }
        }
        break;
    case spatial_type::multipolygon:
        if (flag == intersect_flag::linestring) {
            for (size_t i = 0, num = numobj(); i < num; ++i) {
                if (transform_t::STIntersects<intersect_flag::linestring>(rc, get_subobj(i))) {
                    return true;
                }
            }
        }
        else {
            SDL_ASSERT(flag == intersect_flag::polygon);
            auto const & orient = ring_orient();
            for (size_t i = 0, num = numobj(); i < num; ++i) {
                if (orient[i] == orientation::exterior) {
                    if (transform_t::STIntersects<intersect_flag::polygon>(rc, get_subobj(i))) {
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
    intersect_flag const flag =
        ((type() == spatial_type::polygon) ||
        (type() == spatial_type::multipolygon)) ? intersect_flag::polygon : intersect_flag::linestring;
    return STIntersects(rc, flag);    
}

Meters geo_mem::STDistance(spatial_point const & where) const // = STClosestpoint(where).second
{
    if (is_null()) {
        return 0; 
    }
    if (const size_t num = numobj()) { // multilinestring | multipolygon
        SDL_ASSERT(num > 1);
        if (type() == spatial_type::multipolygon) {
            Meters min_dist = transform_t::STDistance<intersect_flag::polygon>(get_exterior(), where);
            auto const & orient = ring_orient();
            for (size_t i = 1; i < num; ++i) {
                if (orient[i] == orientation::exterior) {
                    const Meters d = transform_t::STDistance<intersect_flag::polygon>(get_subobj(i), where);
                    if (d.value() < min_dist.value()) {
                        min_dist = d;
                    }
                }
            }
            return min_dist;
        }
        else {
            SDL_ASSERT(type() == spatial_type::multilinestring);
            Meters min_dist = transform_t::STDistance<intersect_flag::linestring>(get_exterior(), where);
            for (size_t i = 1; i < num; ++i) {
                const Meters d = transform_t::STDistance<intersect_flag::linestring>(get_subobj(i), where);
                if (d.value() < min_dist.value()) {
                    min_dist = d;
                }
            }
            return min_dist;
        }
    }
    else {
        switch (type()) {
        case spatial_type::point:
            return transform::STDistance(cast_point()->data.point, where);
        case spatial_type::linestring:
            return transform_t::STDistance<intersect_flag::linestring>(*cast_linestring(), where);
        case spatial_type::polygon:
            return transform_t::STDistance<intersect_flag::polygon>(*cast_polygon(), where);
        case spatial_type::linesegment:
            return transform_t::STDistance<intersect_flag::linestring>(*cast_linesegment(), where);
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
    if (is_same(src)) {
        return 0;
    }
    if (src.type() == spatial_type::point) {
        return STDistance(src.cast_point()->data.point);
    }
    SDL_ASSERT(!"STDistance"); // not implemented
    return 0;
}

geo_closest_point_t
geo_mem::STClosestpoint(spatial_point const & where) const
{
    A_STATIC_ASSERT_IS_POD(track_closest_point_t);
    A_STATIC_ASSERT_IS_POD(geo_closest_point_t);

    if (is_null()) {
        return {}; 
    }
    if (const size_t num = numobj()) { // multilinestring | multipolygon
        SDL_ASSERT(num > 1);
        geo_closest_point_t min_dist; // uninitialized
        min_dist.base = transform_t::STClosestpoint(get_exterior(), where);
        min_dist.subobj = 0;
        track_closest_point_t d;
        for (size_t i = 1; i < num; ++i) {
            d = transform_t::STClosestpoint(get_subobj(i), where);
            if (d.distance < min_dist.base.distance) {
                min_dist.base = d;
                min_dist.subobj = i;
            }
        }
        return min_dist;
    }
    else {
        geo_closest_point_t min_dist; // uninitialized
        min_dist.subobj = 0;
        switch (type()) {
        case spatial_type::point:
            min_dist.base = transform_t::STClosestpoint(*cast_point(), where);
            break;
        case spatial_type::linestring:
            min_dist.base = transform_t::STClosestpoint(*cast_linestring(), where);
            break;
        case spatial_type::polygon:
            min_dist.base = transform_t::STClosestpoint(*cast_polygon(), where);
            break;
        case spatial_type::linesegment:
            min_dist.base = transform_t::STClosestpoint(*cast_linesegment(), where);
            break;
        default:
            SDL_ASSERT(0); // not implemented
            return {}; 
        }
        return min_dist;
    }
}

Meters geo_mem::STLength() const
{
    if (is_null()) {
        return 0; 
    }
    switch (type()) {
    case spatial_type::point: 
        return 0;
    case spatial_type::linestring:
        return transform_t::STLength(*cast_linestring());
    case spatial_type::polygon: 
        return transform_t::STLength(*cast_polygon());
    case spatial_type::linesegment:
        return transform_t::STLength(*cast_linesegment());
    case spatial_type::multilinestring:
    case spatial_type::multipolygon:
        if (const size_t num = numobj()) { // multilinestring | multipolygon
            SDL_ASSERT(num > 1);
            Meters length = transform_t::STLength(get_exterior());
            for (size_t i = 1; i < num; ++i) {
                length += transform_t::STLength(get_subobj(i));
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
    switch (type()) {
    case spatial_type::multipolygon:
        return cast_multipolygon()->tail(mem_size(data()));
    case spatial_type::multilinestring:
        return cast_multilinestring()->tail(mem_size(data()));
    default:
        return nullptr;
    }
}

geo_tail const * geo_mem::get_tail_multipolygon() const
{
    if (type() == spatial_type::multipolygon) {
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

void geo_mem::init_ring_orient(unique_vec_orientation & m_ring_orient) const
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
    if (!pdata->m_ring_orient.second) {
        pdata->m_ring_orient.second = true;
        init_ring_orient(pdata->m_ring_orient.first);
    }
    if (pdata->m_ring_orient.first) { 
        return *(pdata->m_ring_orient.first);
    }
    static const vec_orientation empty;
    return empty;
}

geo_mem::vec_winding
geo_mem::ring_winding() const
{
    if (geo_tail const * const tail = get_tail_multipolygon()) {
        const size_t size = tail->size();
        vec_winding result(size, winding::exterior);
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
    switch (type()) {
    case spatial_type::point:           return geometry_types::Point;
    case spatial_type::linestring:      return geometry_types::LineString;
    case spatial_type::linesegment:     return geometry_types::LineString;
    case spatial_type::polygon:         return geometry_types::Polygon;
    case spatial_type::multilinestring: return geometry_types::MultiLineString;
    case spatial_type::multipolygon:
        return multiple_exterior() ? geometry_types::MultiPolygon : geometry_types::Polygon;
    default:
        SDL_ASSERT(type() == spatial_type::null);
        break;
    }
    return geometry_types::Unknown;
}

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
                    SDL_ASSERT(geo_mem().is_null());
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SDL_DEBUG
