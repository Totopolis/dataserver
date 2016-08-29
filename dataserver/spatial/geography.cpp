// geography.cpp
//
#include "common/common.h"
#include "geography.h"
#include "system/page_info.h"

namespace sdl { namespace db { namespace {

// https://en.wikipedia.org/wiki/Point_in_polygon 
// https://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
// Run a semi-infinite ray horizontally (increasing x, fixed y) out from the test point, and count how many edges it crosses. 
// At each crossing, the ray switches between inside and outside. This is called the Jordan curve theorem.
#if 0
template<class float_>
int pnpoly(int const nvert,
           float_ const * const vertx, 
           float_ const * const verty,
           float_ const testx, 
           float_ const testy,
           float_ const epsilon = 0)
{
  int i, j, c = 0;
  for (i = 0, j = nvert-1; i < nvert; j = i++) {
    if ( ((verty[i]>testy) != (verty[j]>testy)) &&
     ((testx + epsilon) < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
       c = !c;
  }
  return c;
}
#endif

//FIXME: 1) check STContains for near poles objects! (high latitude); should switch to curved geometry
//FIXME: 2) for long edges must note spherical curvature, see great circle distance

} // namespace

bool geo_base_polygon::STContains(spatial_point const & test) const 
{
    bool interior = false; // true : point is inside polygon
    auto const _end = this->end();
    auto p1 = this->begin();
    auto p2 = p1 + 1;
    if (*p1 == test) 
        return true;
    while (p2 < _end) {
        SDL_ASSERT(p1 < p2);
        if (*p2 == test)
            return true;
        auto const & v1 = *(p2 - 1);
        auto const & v2 = *p2;
        if (((v1.latitude > test.latitude) != (v2.latitude > test.latitude)) &&
            ((test.longitude + limits::fepsilon) < ((test.latitude - v2.latitude) * 
                (v1.longitude - v2.longitude) / (v1.latitude - v2.latitude) + v2.longitude))) {
            interior = !interior;
        }
        if (*p1 == *p2) { // end of ring found
            ++p2;
            p1 = p2;
        }
        ++p2;
    }
    return interior;
}

size_t geo_base_polygon::ring_num() const
{
    SDL_ASSERT(data.head.tag == spatial_tag::t_multipolygon);
    SDL_ASSERT(size() != 1);
    size_t ring_n = 0;
    auto const _end = this->end();
    auto p1 = this->begin();
    auto p2 = p1 + 1;
    while (p2 < _end) {
        SDL_ASSERT(p1 < p2);
        if (*p1 == *p2) {
            ++ring_n;
            p1 = ++p2;
        }
        ++p2;
    }
    SDL_WARNING(!ring_n || (p1 == _end));
    return (p1 == _end) ? ring_n : 0;
}

//------------------------------------------------------------------------

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
    if (!m_geography) {
        SDL_ASSERT(!m_buf);
        if (m_data.size() == 1) {
            m_geography = m_data[0].first;
        }
        else {
            reset_new(m_buf, make_vector(m_data));
            m_geography = m_buf->data();
        }
    }
    SDL_ASSERT(m_geography);
}

const char * geo_mem::geography() const 
{
    SDL_ASSERT(m_geography);
    return m_geography;
}    

#if SDL_DEBUG && defined(SDL_OS_WIN32)
#define SDL_DEBUG_GEOGRAPHY     0
#endif

spatial_type geo_mem::init_type()
{
    static_assert(sizeof(geo_data) < sizeof(geo_point), "");
    static_assert(sizeof(geo_point) < sizeof(geo_multipolygon), "");
    static_assert(sizeof(geo_pointarray) < sizeof(geo_linesegment), "");
    static_assert(sizeof(geo_multipolygon) == sizeof(geo_pointarray), "");
    static_assert(sizeof(geo_linestring) == sizeof(geo_pointarray), "");

    const size_t data_size = mem_size(this->data());
    SDL_ASSERT(data_size > sizeof(geo_data));

    geo_data const * const data = reinterpret_cast<geo_data const *>(this->geography());
    if (data_size == sizeof(geo_point)) { // 22 bytes
        if (data->data.tag == spatial_tag::t_point) {
#if SDL_DEBUG_GEOGRAPHY
            std::cout << ",Point";
#endif
            return spatial_type::point;
        }
        SDL_ASSERT(0);
        return spatial_type::null;
    }
    if (data_size == sizeof(geo_linesegment)) { // 38 bytes
        if (data->data.tag == spatial_tag::t_linesegment) {
#if SDL_DEBUG_GEOGRAPHY
            std::cout << ",LineString";
            std::cout << ",NULL"; //ring_num
#endif
            return spatial_type::linesegment;
        }
        SDL_ASSERT(0);
        return spatial_type::null;
    }
    if (data_size >= sizeof(geo_pointarray)) { // 26 bytes
        if (data->data.tag == spatial_tag::t_linestring) {
            SDL_ASSERT(!reinterpret_cast<const geo_linestring *>(data)->tail(data_size));
#if SDL_DEBUG_GEOGRAPHY
            std::cout << ",LineString";
            std::cout << ",NULL"; //ring_num
#endif
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
#if SDL_DEBUG_GEOGRAPHY
                        std::cout << ",MultiLineString";
                        std::cout << ",NULL"; //ring_num
#endif
                        SDL_ASSERT(tail->data.reserved.tag == 1); 
                        return spatial_type::multilinestring;
                    }
                    else {
#if SDL_DEBUG_GEOGRAPHY
                        std::cout << ",MultiPolygon"; 
                        std::cout << "," << pp->ring_num();
#endif
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
#if SDL_DEBUG_GEOGRAPHY
                        std::cout << ",LineString";
                        std::cout << ",NULL"; //ring_num
#endif
                        return spatial_type::linestring;
                    }
                    else {
#if SDL_DEBUG_GEOGRAPHY
                        std::cout << ",Polygon";
                        std::cout << "," << pp->ring_num();
#endif
                        SDL_ASSERT(tail->data.numobj.tag == 2);
                        SDL_ASSERT(pp->ring_num() == 1);
                        return spatial_type::polygon;
                    }
                }
            }
            SDL_ASSERT(0);
            return spatial_type::linestring;
        }
    }
    SDL_ASSERT(0);
    return spatial_type::null;
}

std::string geo_mem::STAsText() const {
    switch (m_type) {
    case spatial_type::point:           return to_string::type(*cast_point());
    case spatial_type::polygon:         return to_string::type(*cast_polygon());
    case spatial_type::multipolygon:    return to_string::type(*cast_multipolygon());
    case spatial_type::linesegment:     return to_string::type(*cast_linesegment());
    case spatial_type::linestring:      return to_string::type(*cast_linestring());
    case spatial_type::multilinestring: return to_string::type(*cast_multilinestring());
    default:
        SDL_ASSERT(m_type == spatial_type::null);
        return{};
    }
}

bool geo_mem::STContains(spatial_point const & p) const {
    switch (m_type) {
    case spatial_type::point:           return cast_point()->STContains(p);
    case spatial_type::polygon:         return cast_polygon()->STContains(p);
    case spatial_type::multipolygon:    return cast_multipolygon()->STContains(p);
    case spatial_type::linesegment:     return cast_linesegment()->STContains(p);
    case spatial_type::linestring:      return cast_linestring()->STContains(p);
    case spatial_type::multilinestring: return cast_multilinestring()->STContains(p);
    default:
        SDL_ASSERT(m_type == spatial_type::null);
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

size_t geo_mem::numobj() const
{
    if (m_type == spatial_type::null) {
        SDL_ASSERT(0);
        return 0;
    }
    if (auto tail = get_tail()) {
        return tail->size();
    }
    return 1;
}

geo_mem::unique_access
geo_mem::get_points(size_t const subobj) const
{
    SDL_ASSERT(subobj < numobj());
    switch (m_type) {
    case spatial_type::point:           return make_access(cast_point());
    case spatial_type::linesegment:     return make_access(cast_linesegment());
    case spatial_type::linestring:      return make_access(cast_linestring());
    case spatial_type::polygon:         return make_access(cast_polygon());
    case spatial_type::multipolygon:    return make_access(cast_multipolygon(), subobj);
    case spatial_type::multilinestring: return make_access(cast_multilinestring(), subobj);
    default:
        SDL_ASSERT(0);
        return {};
    }
}

spatial_point const *
geo_mem::subobj_access::begin() const
{
    if (auto const tail = parent.get_tail()) {
        SDL_ASSERT(subobj < tail->size());
        if (subobj) {
            size_t const offset = (*tail)[subobj - 1];
            return m_p->begin() + offset;
        }
        return m_p->begin();
    }
    SDL_ASSERT(0);
    return nullptr;
}

spatial_point const *
geo_mem::subobj_access::end() const
{ 
    if (auto const tail = parent.get_tail()) {
        SDL_ASSERT(subobj < tail->size());
        if (subobj + 1 < tail->size()) {
            size_t const offset = (*tail)[subobj];
            return m_p->begin() + offset;
        }
        return m_p->end();
    }
    SDL_ASSERT(0);
    return nullptr;
}

//------------------------------------------------------------------------

} // db
} // sdl

#if SDL_DEBUG
namespace sdl { namespace db { namespace {
class unit_test {
public:
    unit_test()
    {
        A_STATIC_ASSERT_IS_POD(geo_head);
        A_STATIC_ASSERT_IS_POD(geo_data);
        A_STATIC_ASSERT_IS_POD(geo_point);
        A_STATIC_ASSERT_IS_POD(geo_pointarray);        
        A_STATIC_ASSERT_IS_POD(geo_linesegment);
        A_STATIC_ASSERT_IS_POD(geo_tail);
#if !defined(SDL_VISUAL_STUDIO_2013)
        A_STATIC_ASSERT_IS_POD(geo_linestring);        
        A_STATIC_ASSERT_IS_POD(geo_polygon);
        A_STATIC_ASSERT_IS_POD(geo_multipolygon);
        A_STATIC_ASSERT_IS_POD(geo_multilinestring);
#endif
        static_assert(sizeof(geo_head) == 6, "");
        static_assert(sizeof(geo_data) == 6, "");
        static_assert(sizeof(geo_point) == 22, "");
        static_assert(sizeof(geo_pointarray) == 26, "");
        static_assert(sizeof(geo_polygon) == sizeof(geo_pointarray), "");
        static_assert(sizeof(geo_multipolygon) == sizeof(geo_pointarray), "");
        static_assert(sizeof(geo_linestring) == sizeof(geo_pointarray), "");
        static_assert(sizeof(geo_linesegment) == 38, "");
        static_assert(sizeof(geo_tail::num_type) == 5, "");
        static_assert(sizeof(geo_tail) == 15, "");
        {
            geo_multipolygon test{};
            test.data.head.tag._16 = spatial_tag::t_multipolygon;
            SDL_ASSERT(test.begin() == test.end());
            SDL_ASSERT(test.ring_num() == 0);
            SDL_ASSERT(test.data_mem_size() == sizeof(geo_multipolygon)-sizeof(spatial_point));
            test.data.num_point = 1;
            SDL_ASSERT(test.data_mem_size() == sizeof(geo_multipolygon));
        }
    }
};
static unit_test s_test;
}
} // db
} // sdl
#endif //#if SV_DEBUG

#if 0
    enum twkbGeometryType : std::uint8_t
    {
        twkbPoint = 1,
        twkbLineString = 2,
        twkbPolygon = 3,
        twkbMultiPoint = 4,
        twkbMultiLineString = 5,
        twkbMultiPolygon = 6,
        twkbGeometryCollection = 7
    };
#endif