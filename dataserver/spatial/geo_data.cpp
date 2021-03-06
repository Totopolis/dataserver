// geo_data.cpp
//
#include "dataserver/spatial/geo_data.h"

namespace sdl { namespace db { namespace {

template<class T>
spatial_rect envelope_t(T const & data) {
    const auto end = data.end();
    auto p = data.begin();
    if (p != end) {
        auto rect = spatial_rect::init(*p, *p);
        for (++p; p != end; ++p) {
            set_min(rect.min_lat, p->latitude);
            set_max(rect.max_lat, p->latitude);
            set_min(rect.min_lon, p->longitude);
            set_max(rect.max_lon, p->longitude);
        }
        SDL_ASSERT(rect.is_valid());
        return rect;
    }
    return {};
}

} // namespace

spatial_rect geo_pointarray::envelope() const
{
    return envelope_t(*this);
}

spatial_rect geo_linesegment::envelope() const
{
    return envelope_t(*this);
}

#if 0
size_t geo_base_polygon::ring_num() const
{
    SDL_ASSERT(data.head.tag == spatial_tag::t_multipolygon);
    SDL_ASSERT(size() != 1);
    size_t ring_n = 0;
    auto const _end = this->end();
    auto const _beg = this->begin();
    auto p1 = _beg;
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
#endif

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
        A_STATIC_ASSERT_IS_POD(geo_linestring);        
        A_STATIC_ASSERT_IS_POD(geo_polygon);
        A_STATIC_ASSERT_IS_POD(geo_multipolygon);
        A_STATIC_ASSERT_IS_POD(geo_multilinestring);
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
            SDL_ASSERT_DISABLED(test.ring_num() == 0);
            SDL_ASSERT(test.data_mem_size() == sizeof(geo_multipolygon)-sizeof(spatial_point));
            test.data.num_point = 1;
            SDL_ASSERT(test.data_mem_size() == sizeof(geo_multipolygon));
            SDL_ASSERT(test.envelope().is_null());
        }
    }
};
static unit_test s_test;
}
} // db
} // sdl
#endif //#if SDL_DEBUG

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

//https://www.gaia-gis.it/gaia-sins/spatialite-cookbook/html/wkt-wkb.html
//http://docs.opengeospatial.org/is/12-063r5/12-063r5.html