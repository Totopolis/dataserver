// spatial_type.cpp
//
#include "common/common.h"
#include "spatial_type.h"
#include "page_info.h"
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
    spatial_cell val{};
    val.data.depth = spatial_cell::size;
    return val;
}

spatial_cell spatial_cell::max() {
    static_assert(id_type(-1) == 255, "");
    spatial_cell val;
    for (id_type & id : val.data.id) {
        id = 255;
    }
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

#if SDL_DEBUG
bool spatial_cell::test_depth(spatial_cell const & x) {
    SDL_ASSERT(x.data.depth <= spatial_cell::size);
    for (size_t i = x.data.depth; i < spatial_cell::size; ++i) {
        if (x[i]) {
            SDL_ASSERT(0);
            return false;
        }
    }
    return true;
}
#endif

int spatial_cell::compare(spatial_cell const & x, spatial_cell const & y) {
    SDL_ASSERT(x.data.depth <= size);
    SDL_ASSERT(y.data.depth <= size);
    A_STATIC_ASSERT_TYPE(uint8, id_type);
    uint8 count = a_min(x.data.depth, y.data.depth);
    const uint8 * p1 = x.data.id;
    const uint8 * p2 = y.data.id;
    int v;
    while (count--) {
        if ((v = static_cast<int>(*(p1++)) - static_cast<int>(*(p2++))) != 0) {
            return v;
        }
    }
    return static_cast<int>(x.data.depth) - static_cast<int>(y.data.depth);
}

bool spatial_cell::equal(spatial_cell const & x, spatial_cell const & y) {
    SDL_ASSERT(x.data.depth <= size);
    SDL_ASSERT(y.data.depth <= size);
    A_STATIC_ASSERT_TYPE(uint8, id_type);
    if (x.data.depth != y.data.depth)
        return false;
    uint8 count = x.data.depth;
    const uint8 * p1 = x.data.id;
    const uint8 * p2 = y.data.id;
    while (count--) {
        if (*(p1++) != *(p2++)) {
            return false;
        }
    }
    return true;
}

bool spatial_cell::intersect(spatial_cell const & y) const
{
    const size_t d = a_min(this->depth(), y.depth());
    for (size_t i = 0; i < d; ++i) {
        if ((*this)[i] != y[i])
            return false;
    }
    return true;
}

//-----------------------------------------------------------------------------

#if defined(SDL_VISUAL_STUDIO_2013)
const double spatial_point::min_latitude    = -90;
const double spatial_point::max_latitude    = 90;
const double spatial_point::min_longitude   = -180;
const double spatial_point::max_longitude   = 180;
#endif

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
    return
        spatial_point::valid_latitude(min_lat) &&
        spatial_point::valid_longitude(min_lon) &&
        spatial_point::valid_latitude(max_lat) &&
        spatial_point::valid_longitude(max_lon);
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

void spatial_rect::fill(spatial_point(&dest)[size]) const {
    for (size_t i = 0; i < size; ++i) {
        dest[i] = (*this)[i];
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
                    A_STATIC_ASSERT_IS_POD(spatial_cell);
                    A_STATIC_ASSERT_IS_POD(spatial_point);
                    A_STATIC_ASSERT_IS_POD(spatial_rect);
                    A_STATIC_ASSERT_IS_POD(point_XY<double>);
                    A_STATIC_ASSERT_IS_POD(point_XYZ<double>);
                    A_STATIC_ASSERT_IS_POD(polar_2D);

                    static_assert(sizeof(spatial_cell) == 5, "");
                    static_assert(sizeof(spatial_point) == 16, "");
                    static_assert(sizeof(spatial_rect) == 32, "");
                    static_assert(sizeof(spatial_grid) == 4, "");

                    static_assert(is_power_2<1>::value, "");
                    static_assert(is_power_2<spatial_grid::LOW>::value, "");
                    static_assert(is_power_2<spatial_grid::MEDIUM>::value, "");
                    static_assert(is_power_2<spatial_grid::HIGH>::value, "");
        
                    SDL_ASSERT(is_power_two(spatial_grid::LOW));
                    SDL_ASSERT(is_power_two(spatial_grid::MEDIUM));
                    SDL_ASSERT(is_power_two(spatial_grid::HIGH));
                    SDL_ASSERT(fatan2(limits::fepsilon, limits::fepsilon) == 0);
                    {
                        spatial_cell x{};
                        spatial_cell y{};
                        SDL_ASSERT(!(x < y));
                        SDL_ASSERT(x == y);
                        SDL_ASSERT(x.intersect(x));
                        SDL_ASSERT(x.intersect(y));
                        x = spatial_cell::min();
                        y = spatial_cell::max();
                        SDL_ASSERT(x.id32() == 0);
                        SDL_ASSERT(y.id32() == 0xFFFFFFFF);
                        SDL_ASSERT(x == spatial_cell::min());
                        SDL_ASSERT(y == spatial_cell::max());
                        SDL_ASSERT(x < y);
                        SDL_ASSERT(x != y);
                        SDL_ASSERT(!x.intersect(y));
                        x = y;
                        x.data.depth = 1;
                        SDL_ASSERT(x != y);
                        SDL_ASSERT(x.intersect(y));
                    }
                    SDL_ASSERT(to_string::type_less(spatial_cell::parse_hex("6ca5f92a04")) == "108-165-249-42-4");
                    {
                        spatial_cell x{}, y{};
                        SDL_ASSERT(spatial_cell::compare(x, y) == 0);
                        SDL_ASSERT(x == y);
                        y.set_depth(1);
                        SDL_ASSERT(x != y);
                        SDL_ASSERT(spatial_cell::compare(x, y) < 0);
                        SDL_ASSERT(spatial_cell::compare(y, x) > 0);
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
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
