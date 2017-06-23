// hilbert.inl
//
#pragma once
#ifndef __SDL_SPATIAL_HILBERT_INL__
#define __SDL_SPATIAL_HILBERT_INL__

namespace sdl { namespace db { namespace hilbert {

#define is_static_hilbert  1
#if is_static_hilbert 
static const point_XY<uint8> static_d2xy[spatial_grid::HIGH_HIGH] = {
{0,0},{1,0},{1,1},{0,1},{0,2},{0,3},{1,3},{1,2},{2,2},{2,3},{3,3},{3,2},{3,1},{2,1},{2,0},{3,0}, // 0
{4,0},{4,1},{5,1},{5,0},{6,0},{7,0},{7,1},{6,1},{6,2},{7,2},{7,3},{6,3},{5,3},{5,2},{4,2},{4,3}, // 1
{4,4},{4,5},{5,5},{5,4},{6,4},{7,4},{7,5},{6,5},{6,6},{7,6},{7,7},{6,7},{5,7},{5,6},{4,6},{4,7}, // 2
{3,7},{2,7},{2,6},{3,6},{3,5},{3,4},{2,4},{2,5},{1,5},{1,4},{0,4},{0,5},{0,6},{1,6},{1,7},{0,7}, // 3
{0,8},{0,9},{1,9},{1,8},{2,8},{3,8},{3,9},{2,9},{2,10},{3,10},{3,11},{2,11},{1,11},{1,10},{0,10},{0,11}, // 4
{0,12},{1,12},{1,13},{0,13},{0,14},{0,15},{1,15},{1,14},{2,14},{2,15},{3,15},{3,14},{3,13},{2,13},{2,12},{3,12}, // 5
{4,12},{5,12},{5,13},{4,13},{4,14},{4,15},{5,15},{5,14},{6,14},{6,15},{7,15},{7,14},{7,13},{6,13},{6,12},{7,12}, // 6
{7,11},{7,10},{6,10},{6,11},{5,11},{4,11},{4,10},{5,10},{5,9},{4,9},{4,8},{5,8},{6,8},{6,9},{7,9},{7,8}, // 7
{8,8},{8,9},{9,9},{9,8},{10,8},{11,8},{11,9},{10,9},{10,10},{11,10},{11,11},{10,11},{9,11},{9,10},{8,10},{8,11}, // 8
{8,12},{9,12},{9,13},{8,13},{8,14},{8,15},{9,15},{9,14},{10,14},{10,15},{11,15},{11,14},{11,13},{10,13},{10,12},{11,12}, // 9
{12,12},{13,12},{13,13},{12,13},{12,14},{12,15},{13,15},{13,14},{14,14},{14,15},{15,15},{15,14},{15,13},{14,13},{14,12},{15,12}, // 10
{15,11},{15,10},{14,10},{14,11},{13,11},{12,11},{12,10},{13,10},{13,9},{12,9},{12,8},{13,8},{14,8},{14,9},{15,9},{15,8}, // 11
{15,7},{14,7},{14,6},{15,6},{15,5},{15,4},{14,4},{14,5},{13,5},{13,4},{12,4},{12,5},{12,6},{13,6},{13,7},{12,7}, // 12
{11,7},{11,6},{10,6},{10,7},{9,7},{8,7},{8,6},{9,6},{9,5},{8,5},{8,4},{9,4},{10,4},{10,5},{11,5},{11,4}, // 13
{11,3},{11,2},{10,2},{10,3},{9,3},{8,3},{8,2},{9,2},{9,1},{8,1},{8,0},{9,0},{10,0},{10,1},{11,1},{11,0}, // 14
{12,0},{13,0},{13,1},{12,1},{12,2},{12,3},{13,3},{13,2},{14,2},{14,3},{15,3},{15,2},{15,1},{14,1},{14,0},{15,0}, // 15
};

static const uint8 static_xy2d[spatial_grid::HIGH][spatial_grid::HIGH] = { // [X][Y]
{0,3,4,5,58,59,60,63,64,65,78,79,80,83,84,85}, // 0
{1,2,7,6,57,56,61,62,67,66,77,76,81,82,87,86}, // 1
{14,13,8,9,54,55,50,49,68,71,72,75,94,93,88,89}, // 2
{15,12,11,10,53,52,51,48,69,70,73,74,95,92,91,90}, // 3
{16,17,30,31,32,33,46,47,122,121,118,117,96,99,100,101}, // 4
{19,18,29,28,35,34,45,44,123,120,119,116,97,98,103,102}, // 5
{20,23,24,27,36,39,40,43,124,125,114,115,110,109,104,105}, // 6
{21,22,25,26,37,38,41,42,127,126,113,112,111,108,107,106}, // 7
{234,233,230,229,218,217,214,213,128,129,142,143,144,147,148,149}, // 8
{235,232,231,228,219,216,215,212,131,130,141,140,145,146,151,150}, // 9
{236,237,226,227,220,221,210,211,132,135,136,139,158,157,152,153}, // 10
{239,238,225,224,223,222,209,208,133,134,137,138,159,156,155,154}, // 11
{240,243,244,245,202,203,204,207,186,185,182,181,160,163,164,165}, // 12
{241,242,247,246,201,200,205,206,187,184,183,180,161,162,167,166}, // 13
{254,253,248,249,198,199,194,193,188,189,178,179,174,173,168,169}, // 14
{255,252,251,250,197,196,195,192,191,190,177,176,175,172,171,170}, // 15
};
static_assert(sizeof(hilbert::static_d2xy) == 256 * 2, "");
static_assert(sizeof(hilbert::static_xy2d) == 256, "");
#endif

//https://en.wikipedia.org/wiki/Hilbert_curve
// The following code performs the mappings in both directions, 
// using iteration and bit operations rather than recursion. 
// It assumes a square divided into n by n cells, for n a power of 2,
// with integer coordinates, with (0,0) in the lower left corner, (n-1,n-1) in the upper right corner,
// and a distance d that starts at 0 in the lower left corner and goes to n^2-1 in the lower-right corner.

//rotate/flip a quadrant appropriately
inline void rot(const int n, int & x, int & y, const int rx, const int ry) {
    SDL_ASSERT(is_power_two(n));
    SDL_ASSERT((rx == 0) || (rx == 1));
    SDL_ASSERT((ry == 0) || (ry == 1));
    if (ry == 0) {
        if (rx == 1) {
            x = n - 1 - x;
            y = n - 1 - y;
        }
        //Swap x and y
        auto t = x;
        x = y;
        y = t;
    }
}

//convert (x,y) to d
inline int xy2d(const int n, int x, int y) {
    SDL_ASSERT(is_power_two(n));
    SDL_ASSERT(x < n);
    SDL_ASSERT(y < n);
    int rx, ry, d = 0;
    for (int s = n/2; s > 0; s /= 2) {
        rx = (x & s) > 0;
        ry = (y & s) > 0;
        d += s * s * ((3 * rx) ^ ry);
        rot(s, x, y, rx, ry);
    }
    SDL_ASSERT((d >= 0) && (d < (n * n)));
    SDL_ASSERT(d < spatial_grid::HIGH_HIGH); // to be compatible with spatial_cell::id_type
    return d;
}

//convert (x,y) to d
template<int n>
inline int n_xy2d(int x, int y) {
    static_assert(n == 16, "spatial_grid::HIGH");
    static_assert(is_power_two(n), "");
    SDL_ASSERT(x < n);
    SDL_ASSERT(y < n);
    int rx, ry, d = 0;
    for (int s = n/2; s > 0; s /= 2) {
        rx = (x & s) > 0;
        ry = (y & s) > 0;
        d += s * s * ((3 * rx) ^ ry);
        rot(s, x, y, rx, ry);
    }
    SDL_ASSERT((d >= 0) && (d < (n * n)));
    SDL_ASSERT(d < spatial_grid::HIGH_HIGH); // to be compatible with spatial_cell::id_type
    return d;
}

//convert d to (x,y)
inline void d2xy(const int n, const int d, int & x, int & y) {
    SDL_ASSERT(is_power_two(n));
    SDL_ASSERT((d >= 0) && (d < (n * n)));
    int rx, ry, t = d;
    x = y = 0;
    for (int s = 1; s < n; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        rot(s, x, y, rx, ry);
        x += s * rx;
        y += s * ry;
        t /= 4;
    }
    SDL_ASSERT((x >= 0) && (x < n));
    SDL_ASSERT((y >= 0) && (y < n));
}

//convert d to (x,y)
template<const int n>
inline void n_d2xy(const int d, int & x, int & y) {
    static_assert(n == 16, "spatial_grid::HIGH");
    SDL_ASSERT(is_power_two(n));
    SDL_ASSERT((d >= 0) && (d < (n * n)));
    int rx, ry, t = d;
    x = y = 0;
    for (int s = 1; s < n; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        rot(s, x, y, rx, ry);
        x += s * rx;
        y += s * ry;
        t /= 4;
    }
    SDL_ASSERT((x >= 0) && (x < n));
    SDL_ASSERT((y >= 0) && (y < n));
}

#if is_static_hilbert 
template<typename T>
inline T s_xy2d(const point_XY<int> & p) {
    A_STATIC_ASSERT_TYPE(T, spatial_cell::id_type);
    SDL_ASSERT(p.X < 16);
    SDL_ASSERT(p.Y < 16);
    return static_cast<T>(static_xy2d[p.X][p.Y]);
}
#endif

template<typename T>
inline T xy2d(const int n, const point_XY<int> & p) {
    A_STATIC_ASSERT_TYPE(T, spatial_cell::id_type);
    return static_cast<T>(xy2d(n, p.X, p.Y));
}


template<int n, typename T>
inline T n_xy2d(const point_XY<int> & p) {
    A_STATIC_ASSERT_TYPE(T, spatial_cell::id_type);
    static_assert(n == 16, "spatial_grid::HIGH");
    return static_cast<T>(n_xy2d<n>(p.X, p.Y));
}

template<typename T>
inline point_XY<int> d2xy(const int n, T const d) {
    A_STATIC_ASSERT_TYPE(T, spatial_cell::id_type);
    point_XY<int> p;
    d2xy(n, d, p.X, p.Y);
    return p;
}

template<int n, typename T>
inline point_XY<int> n_d2xy(T const d) {
    A_STATIC_ASSERT_TYPE(T, spatial_cell::id_type);
    static_assert(n == 16, "spatial_grid::HIGH");
    point_XY<int> p;
    n_d2xy<n>(d, p.X, p.Y);
    return p;
}


} // hilbert
} // db
} // sdl

#endif // __SDL_SPATIAL_HILBERT_INL__