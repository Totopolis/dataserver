// static.cpp
//
#include "common.h"
#include "static.h"

namespace sdl {

#if defined(SDL_VISUAL_STUDIO_2013)
const double limits::fepsilon = 1e-12;
const double limits::PI = 3.14159265358979323846;
const double limits::RAD_TO_DEG = 57.295779513082321;
const double limits::DEG_TO_RAD = 0.017453292519943296;
const double limits::SQRT_2 = 1.41421356237309504880;       // = sqrt(2)
const double limits::ATAN_1_2 = 0.46364760900080609;        // = std::atan2(1, 2)
const double limits::EARTH_RADIUS = 6371000;                // in meters
const double limits::EARTH_MAJOR_RADIUS = 6378137;          // in meters, WGS 84, Semi-major axis
const double limits::EARTH_MINOR_RADIUS = 6356752.314245;   // in meters, WGS 84, Semi-minor axis
const double limits::EARTH_MINOR_ARC = EARTH_MINOR_RADIUS * DEG_TO_RAD; // 1 degree arc in meters
#endif

} // sdl

#if SDL_DEBUG
namespace sdl { namespace {
    class unit_test {
    public:
        unit_test() {
            static_assert(is_odd(1), "");
            static_assert(is_odd(-1), "");
            static_assert(!is_odd(2), "");
            static_assert(!is_odd(-2), "");
        }
    };
    static unit_test s_test;
}} // sdl
#endif //#if SV_DEBUG
