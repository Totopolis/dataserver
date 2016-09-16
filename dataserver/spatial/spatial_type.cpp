// spatial_type.cpp
//
#include "common/common.h"
#include "spatial_type.h"
#include "system/page_info.h"
#include <cmath>

#if 0
/* Compare at most COUNT bytes starting at PTR1,PTR2.
 * Returns 0 IFF all COUNT bytes are equal.
 */
EXTERN_C int __cdecl memcmp(const void *Ptr1, const void *Ptr2, size_t Count)
{
    INT v = 0;
    BYTE *p1 = (BYTE *)Ptr1;
    BYTE *p2 = (BYTE *)Ptr2;

    while(Count-- > 0 && v == 0) {
        v = *(p1++) - *(p2++);
    }

    return v;
}
#endif

namespace sdl { namespace db {

polar_2D polar_2D::polar(point_2D const & s) {
    polar_2D p;
    p.radial = std::sqrt(s.X * s.X + s.Y * s.Y);
    p.arg = fatan2(s.Y, s.X);
    return p;
}

spatial_cell spatial_cell::min() {
    spatial_cell val;
    val.data.id._32 = 0;
    val.data.depth = spatial_cell::size;
    return val;
}

spatial_cell spatial_cell::max() {
    static_assert(id_type(-1) == 255, "");
    spatial_cell val;
    val.data.id._32 = 0xFFFFFFFF;
    val.data.depth = spatial_cell::size;
    return val;
}

spatial_cell spatial_cell::parse_hex(const char * const str)
{
    if (is_str_valid(str)) {
        uint64 hex = ::strtoll(str, nullptr, 16);
        spatial_cell cell{};
        size_t i = 0;
        while (hex) {
            enum { len = A_ARRAY_SIZE(cell.raw)-1 };
            const uint8 b = hex & 0xFF;
            hex >>= 8;
            cell.raw[len - i] = (char) b;
            if (i++ == len) {
                SDL_ASSERT(!hex);
                break;
            }
        }
        SDL_ASSERT(cell);
        return cell;
    }
    SDL_ASSERT(0);
    return{};
}

#if 0 // don't remove this code
bool spatial_cell::less(spatial_cell const & x, spatial_cell const & y) {
    A_STATIC_ASSERT_TYPE(uint8, id_type);
    SDL_ASSERT(x.data.depth <= size);
    SDL_ASSERT(y.data.depth <= size);
    uint8 count = a_min(x.data.depth, y.data.depth);
    const uint8 * p1 = x.data.id.cell;
    const uint8 * p2 = y.data.id.cell;
    int v;
    while (count--) {
        if ((v = static_cast<int>(*(p1++)) - static_cast<int>(*(p2++))) != 0) {
            return v < 0;
        }
    }
    return x.data.depth < y.data.depth;
}

bool spatial_cell::equal(spatial_cell const & x, spatial_cell const & y) {
    A_STATIC_ASSERT_TYPE(uint8, id_type);
    SDL_ASSERT(x.data.depth <= size);
    SDL_ASSERT(y.data.depth <= size);
    if (x.data.depth != y.data.depth)
        return false;
    uint8 count = x.data.depth;
    const uint8 * p1 = x.data.id.cell;
    const uint8 * p2 = y.data.id.cell;
    while (count--) {
        if (*(p1++) != *(p2++)) {
            return false;
        }
    }
    return true;
}
#endif

bool spatial_cell::intersect(spatial_cell const & y) const {
    const size_t d = a_min(data.depth, y.data.depth);
    for (size_t i = 0; i < d; ++i) {
        if ((*this)[i] != y[i])
            return false;
    }
    return true;
}

//-----------------------------------------------------------------------------

double spatial_point::norm_longitude(double x) { // wrap around meridian +/-180
    if (x > 180) {
        do {
            x -= 360;
        } while (x > 180);
    }
    else if (x < -180) {
        do {
            x += 360;
        } while (x < -180);
    }
    SDL_ASSERT(valid_longitude(x));
    return x;        
}

double spatial_point::norm_latitude(double x) { // wrap around poles +/-90
    if (x > 180) {
        do {
            x -= 360;
        } while (x > 180);
    }
    else if (x < -180) {
        do {
            x += 360;
        } while (x < -180);
    }
    SDL_ASSERT(frange(x, -180, 180));
    if (x > 90) {
        do {
            x = 180 - x;
        } while (x > 90);
    }
    else if (x < -90) {
        do {
            x = -180 - x;
        } while (x < -90);
    }
    SDL_ASSERT(valid_latitude(x));
    return x;        
}

bool spatial_point::match(spatial_point const & p) const
{
    if (!fequal(latitude, p.latitude)) {
        return false;
    }
    if (!fequal(longitude, p.longitude)) {
        if (fequal(a_abs(latitude), 90)) {
            return true;
        }
        if (fequal(a_abs(longitude), 180) && fequal(a_abs(p.longitude), 180)) {
            return true;
        }
        return false;
    }
    return true;
}

spatial_point spatial_point::STPointFromText(const std::string & s) // POINT (longitude latitude)
{
   //SDL_ASSERT(setlocale_t::get() == "en-US");
   if (!s.empty()) {
        const size_t p1 = s.find('(');
        if (p1 != std::string::npos) {
            const size_t p2 = s.find(' ', p1);
            if (p2 != std::string::npos) {
                const size_t p3 = s.find(')', p2);
                if (p3 != std::string::npos) {
                    std::string const s1 = s.substr(p1 + 1, p2 - p1 - 1);
                    std::string const s2 = s.substr(p2 + 1, p3 - p2 - 1);
                    const Longitude lon = atof(s1.c_str()); // Note. atof depends on locale for decimal point character ('.' or ',')
                    const Latitude lat = atof(s2.c_str());
                    return spatial_point::init(lat, lon);
                }
            }
        }
    }
    SDL_ASSERT(0);
    return {};
}

//-----------------------------------------------------------------------------

bool spatial_rect::is_valid() const {
    SDL_ASSERT(min_lat <= max_lat);
    return
        spatial_point::valid_latitude(min_lat) &&
        spatial_point::valid_longitude(min_lon) &&
        spatial_point::valid_latitude(max_lat) &&
        spatial_point::valid_longitude(max_lon);
}

bool spatial_rect::equal(spatial_rect const & rc) const {
    return
        fequal(min_lat, rc.min_lat) &&
        fequal(min_lon, rc.min_lon) &&
        fequal(max_lat, rc.max_lat) &&
        fequal(max_lon, rc.max_lon);
}

spatial_point spatial_rect::operator[](size_t const i) const { // counter-clock wize
    SDL_ASSERT(is_valid());
    SDL_ASSERT(i < this->size);
    switch (i) {
    default: // i = 0
            return spatial_point::init(min_lat, min_lon);
    case 1: return spatial_point::init(min_lat, max_lon);
    case 2: return spatial_point::init(max_lat, max_lon);
    case 3: return spatial_point::init(max_lat, min_lon);    
    }
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
                    A_STATIC_ASSERT_IS_POD(spatial_tag);
                    A_STATIC_ASSERT_IS_POD(spatial_cell);
                    A_STATIC_ASSERT_IS_POD(spatial_point);
                    A_STATIC_ASSERT_IS_POD(spatial_rect);
                    A_STATIC_ASSERT_IS_POD(point_XY<double>);
                    A_STATIC_ASSERT_IS_POD(point_XYZ<double>);
                    A_STATIC_ASSERT_IS_POD(polar_2D);
                    A_STATIC_ASSERT_IS_POD(swap_point_2D<false>);
                    static_assert(sizeof(spatial_tag) == 2, "");
                    static_assert(sizeof(swap_point_2D<false>) == sizeof(point_2D), "");
                    static_assert(sizeof(spatial_cell::id_array) == sizeof(uint32), "");
                    static_assert(sizeof(spatial_cell) == 5, "");
                    static_assert(sizeof(spatial_point) == 16, "");
                    static_assert(sizeof(spatial_rect) == 32, "");
#if high_grid_optimization
                    static_assert(sizeof(spatial_grid) == 1, "");
#else
                    static_assert(sizeof(spatial_grid) == 4, "");
#endif
                    static_assert(is_power_2<1>::value, "");
                    static_assert(is_power_2<spatial_grid::LOW>::value, "");
                    static_assert(is_power_2<spatial_grid::MEDIUM>::value, "");
                    static_assert(is_power_2<spatial_grid::HIGH>::value, "");
                    //static_assert(std::is_pod<array_t<spatial_point, 4>>::value, "array_t");
        
                    SDL_ASSERT(is_power_two(spatial_grid::LOW));
                    SDL_ASSERT(is_power_two(spatial_grid::MEDIUM));
                    SDL_ASSERT(is_power_two(spatial_grid::HIGH));
                    if (1) {
                        SDL_ASSERT(fatan2(limits::fepsilon, limits::fepsilon) == 0);
                        SDL_ASSERT(fatan2(0, 0) == 0);
                        SDL_ASSERT_1(fequal(fatan2(1, 0), limits::PI / 2));
                        SDL_ASSERT_1(fequal(fatan2(-1, 0), -limits::PI / 2));
                    }
                    {
                        spatial_cell x{};
                        spatial_cell y{};
                        SDL_ASSERT(!(x < y));
                        SDL_ASSERT(x == y);
                        SDL_ASSERT(x.intersect(x));
                        SDL_ASSERT(x.intersect(y));
                        x = spatial_cell::min();
                        y = spatial_cell::max();
                        spatial_cell z = spatial_cell::max();
                        z[3] = 0;
                        SDL_ASSERT(z < y);
                        SDL_ASSERT(z != y);
                        z = spatial_cell::max();
                        SDL_ASSERT(z.zero_tail());
                        z = spatial_cell::set_depth(z, 3);
                        SDL_ASSERT(z.zero_tail());
                        SDL_ASSERT(z < y);
                        SDL_ASSERT(z != y);
                        z = spatial_cell::set_depth(z, 1);
                        SDL_ASSERT(z.zero_tail());
                        z = spatial_cell::set_depth(z, 0);
                        SDL_ASSERT(z.zero_tail());
                        SDL_ASSERT(x.data.id._32 == 0);
                        SDL_ASSERT(y.data.id._32 == 0xFFFFFFFF);
                        SDL_ASSERT(x == spatial_cell::min());
                        SDL_ASSERT(y == spatial_cell::max());
                        SDL_ASSERT(x < y);
                        SDL_ASSERT(x != y);
                        SDL_ASSERT(!x.intersect(y));
                        x = spatial_cell::set_depth(y, 1);
                        SDL_ASSERT(x != y);
                        SDL_ASSERT(x.intersect(y));
                    }
                    SDL_ASSERT(to_string::type_less(spatial_cell::parse_hex("6ca5f92a04")) == "108-165-249-42-4");
                    {
                        spatial_cell x{}, y{};
                        SDL_ASSERT(!spatial_cell::less(x, y));
                        SDL_ASSERT(x == y);
                        y = spatial_cell::set_depth(y, 1);
                        SDL_ASSERT(x != y);
                        SDL_ASSERT(spatial_cell::less(x, y));
                        SDL_ASSERT(!spatial_cell::less(y, x));
                    }
                    {
                        SDL_ASSERT_1(point_2D{0.5, - 0.25} == (point_2D{1, 0} - point_2D{0.5, 0.25}));
                        polar_2D p = polar(point_2D{1, 0});
                        SDL_ASSERT(fequal(p.arg, 0));
                        p = polar(point_2D{1, 0} - point_2D{0.5, 0.25});
                        SDL_ASSERT(fequal(p.arg, -limits::ATAN_1_2));
                        p = polar(point_2D{-1, 0});
                        SDL_ASSERT(fequal(p.arg, limits::PI));
                        p = polar(point_2D{ 1, 1 });
                        SDL_ASSERT(fequal(p.radial, sqrt(2.0)));
                        SDL_ASSERT(fequal(p.arg, limits::PI/4));
                        p = polar(point_2D{ -1, 1 });
                        SDL_ASSERT(fequal(p.arg, limits::PI * 3/4));
                        p = polar(point_2D{ -1, -1 });
                        SDL_ASSERT(fequal(p.arg, limits::PI * -3/4));
                        p = polar(point_2D{ 1, -1 });
                        SDL_ASSERT(fequal(p.arg, limits::PI * -1/4));
                    }
                    {
                        SP p1 = SP::init(Latitude(45), Longitude(180));
                        SP p2 = SP::init(Latitude(45), Longitude(-180));
                        SDL_ASSERT(p1.match(p2));
                        p1 = SP::init(Latitude(90), Longitude(180));
                        p2 = SP::init(Latitude(90), Longitude(45));
                        SDL_ASSERT(p1.match(p2));
                    }
                    static_assert(cell_capacity<spatial_cell::depth_4>::value == 256, "");
                    static_assert(cell_capacity<spatial_cell::depth_3>::value == 256 * 256, "");
                    static_assert(cell_capacity<spatial_cell::depth_2>::value == 256 * 256 * 256, "");
                    static_assert(cell_capacity<spatial_cell::depth_1>::value64 == uint64(256) * 256 * 256 * 256, "");
                    static_assert(sizeof(spatial_grid_high<true>) == 1, "");
                    static_assert(sizeof(spatial_grid_high<false>) == 4, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

/*inline constexpr uint32 spatial_cell::capacity(size_t const depth) {
    return uint32(~(uint64(0xFFFFFF00) << ((4 - depth) << 3)));
}*/