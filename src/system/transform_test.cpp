// transform_test.cpp
//
#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    test_hilbert();
                    test_spatial();
                    {
                        A_STATIC_ASSERT_TYPE(point_2D::type, double);
                        A_STATIC_ASSERT_TYPE(point_3D::type, double);                        
                        SDL_ASSERT_1(math::cartesian(Latitude(0), Longitude(0)) == point_3D{1, 0, 0});
                        SDL_ASSERT_1(math::cartesian(Latitude(0), Longitude(90)) == point_3D{0, 1, 0});
                        SDL_ASSERT_1(math::cartesian(Latitude(90), Longitude(0)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(math::cartesian(Latitude(90), Longitude(90)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(math::cartesian(Latitude(45), Longitude(45)) == point_3D{0.5, 0.5, 0.70710678118654752440});
                        SDL_ASSERT_1(math::line_plane_intersect(Latitude(0), Longitude(0)) == point_3D{1, 0, 0});
                        SDL_ASSERT_1(math::line_plane_intersect(Latitude(0), Longitude(90)) == point_3D{0, 1, 0});
                        SDL_ASSERT_1(math::line_plane_intersect(Latitude(90), Longitude(0)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(math::line_plane_intersect(Latitude(90), Longitude(90)) == point_3D{0, 0, 1});
                        SDL_ASSERT_1(fequal(length(math::line_plane_intersect(Latitude(45), Longitude(45))), 0.58578643762690497));
                        SDL_ASSERT_1(math::longitude_quadrant(0) == 0);
                        SDL_ASSERT_1(math::longitude_quadrant(45) == 0);
                        SDL_ASSERT_1(math::longitude_quadrant(90) == 1);
                        SDL_ASSERT_1(math::longitude_quadrant(135) == 1);
                        SDL_ASSERT_1(math::longitude_quadrant(180) == 2);
                        SDL_ASSERT_1(math::longitude_quadrant(-45) == 0);
                        SDL_ASSERT_1(math::longitude_quadrant(-90) == 3);
                        SDL_ASSERT_1(math::longitude_quadrant(-135) == 3);
                        SDL_ASSERT_1(math::longitude_quadrant(-180) == 2);
                        SDL_ASSERT(fequal(limits::ATAN_1_2, std::atan2(1, 2)));
                        static_assert(fsign(0) == 0, "");
                        static_assert(fsign(1) == 1, "");
                        static_assert(fsign(-1) == -1, "");
                        static_assert(fzero(0), "");
                        static_assert(fzero(limits::fepsilon), "");
                        static_assert(!fzero(limits::fepsilon * 2), "");
                    }
                    {
                        spatial_cell x{}, y{};
                        SDL_ASSERT(spatial_cell::compare(x, y) == 0);
                        SDL_ASSERT(x == y);
                        y.set_depth(1);
                        SDL_ASSERT(x != y);
                        SDL_ASSERT(spatial_cell::compare(x, y) < 0);
                        SDL_ASSERT(spatial_cell::compare(y, x) > 0);
                    }
                    if (0) { // generate static tables
                        std::cout << "\nd2xy:\n";
                        enum { HIGH = spatial_grid::HIGH };
                        int dist[HIGH][HIGH]{};
                        for (int i = 0; i < HIGH; ++i) {
                            for (int j = 0; j < HIGH; ++j) {
                                const int d = i * HIGH + j;
                                point_XY<int> const h = transform::d2xy(d);
                                dist[h.X][h.Y] = d;
                                std::cout << "{" << h.X << "," << h.Y << "},";
                            }
                            std::cout << " // " << i << "\n";
                        }
                        std::cout << "\nxy2d:";
                        for (int x = 0; x < HIGH; ++x) {
                            std::cout << "\n{";
                            for (int y = 0; y < HIGH; ++y) {
                                if (y) std::cout << ",";
                                std::cout << dist[x][y];
                            }
                            std::cout << "}, // " << x;
                        }
                        std::cout << std::endl;
                    }
                    if (1) {
                        SDL_ASSERT(fequal(math::norm_longitude(0), 0));
                        SDL_ASSERT(fequal(math::norm_longitude(180), 180));
                        SDL_ASSERT(fequal(math::norm_longitude(-180), -180));
                        SDL_ASSERT(fequal(math::norm_longitude(-180 - 90), 90));
                        SDL_ASSERT(fequal(math::norm_longitude(180 + 90), -90));
                        SDL_ASSERT(fequal(math::norm_longitude(180 + 90 + 360), -90));
                        SDL_ASSERT(fequal(math::norm_latitude(0), 0));
                        SDL_ASSERT(fequal(math::norm_latitude(-90), -90));
                        SDL_ASSERT(fequal(math::norm_latitude(90), 90));
                        SDL_ASSERT(fequal(math::norm_latitude(90+10), 80));
                        SDL_ASSERT(fequal(math::norm_latitude(90+10+360), 80));
                        SDL_ASSERT(fequal(math::norm_latitude(-90-10), -80));
                        SDL_ASSERT(fequal(math::norm_latitude(-90-10-360), -80));
                        SDL_ASSERT(fequal(math::norm_latitude(-90-10+360), -80));
                    }
                    if (1) {
                        SDL_ASSERT(math::point_quadrant(point_2D{}) == 1);
                        SDL_ASSERT(math::point_quadrant(point_2D{0, 0.25}) == 2);
                        SDL_ASSERT(math::point_quadrant(point_2D{0.5, 0.375}) == 3);
                        SDL_ASSERT(math::point_quadrant(point_2D{0.5, 0.5}) == 3);
                        SDL_ASSERT(math::point_quadrant(point_2D{1.0, 0.25}) == 0);
                        SDL_ASSERT(math::point_quadrant(point_2D{1.0, 0.75}) == 0);
                        SDL_ASSERT(math::point_quadrant(point_2D{1.0, 1.0}) == 0);
                        SDL_ASSERT(math::point_quadrant(point_2D{0.5, 1.0}) == 1);
                        SDL_ASSERT(math::point_quadrant(point_2D{0, 0.75}) == 2);
                    }
                    if (1) {
                        Meters const d1 = math::earth_radius(Latitude(0)) * limits::PI / 2;
                        Meters const d2 = d1.value() / 2;
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d1, Degree(0)).equal(Latitude(90), Longitude(0)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d1, Degree(360)).equal(Latitude(90), Longitude(0)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(0)).equal(Latitude(45), Longitude(0)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(90)).equal(Latitude(0), Longitude(45)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(180)).equal(Latitude(-45), Longitude(0)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(0), Longitude(0)), d2, Degree(270)).equal(Latitude(0), Longitude(-45)));
                        SDL_ASSERT(math::destination(SP::init(Latitude(90), Longitude(0)), d2, Degree(0)).equal(Latitude(45), Longitude(0)));
                    }
                    if (0) {
                        draw_grid();
                    }
                }
            private:
                static void trace_hilbert(const int n) {
                    for (int y = 0; y < n; ++y) {
                        std::cout << y;
                        for (int x = 0; x < n; ++x) {
                            const int d = hilbert::xy2d(n, x, y);
                            std::cout << "," << d;
                        }
                        std::cout << std::endl;
                    }
                }
                static void test_hilbert(const int n) {
                    for (int d = 0; d < (n * n); ++d) {
                        int x = 0, y = 0;
                        hilbert::d2xy(n, d, x, y);
                        SDL_ASSERT(d == hilbert::xy2d(n, x, y));
                        //SDL_TRACE("d2xy: n = ", n, " d = ", d, " x = ", x, " y = ", y);
#if is_static_hilbert
                        if (n == spatial_grid::HIGH) {
                            SDL_ASSERT(hilbert::static_d2xy[d].X == x);
                            SDL_ASSERT(hilbert::static_d2xy[d].Y == y);
                            SDL_ASSERT(hilbert::static_xy2d[x][y] == d);
                        }
#endif
                    }
                }
                static void test_hilbert() {
                    spatial_grid::grid_size const sz = spatial_grid::HIGH;
                    for (int i = 0; (1 << i) <= sz; ++i) {
                        test_hilbert(1 << i);
                    }
                }
                static void trace_cell(const spatial_cell & cell) {
                    //SDL_TRACE(to_string::type(cell));
                }
                static void test_spatial(const spatial_grid & grid) {
                    if (1) {
                        spatial_point p1{}, p2{};
                        for (int i = 0; i <= 4; ++i) {
                        for (int j = 0; j <= 2; ++j) {
                            p1.longitude = 45 * i; 
                            p2.longitude = -45 * i;
                            p1.latitude = 45 * j;
                            p2.latitude = -45 * j;
                            math::project_globe(p1);
                            math::project_globe(p2);
                            transform::make_cell(p1, spatial_grid(spatial_grid::LOW));
                            transform::make_cell(p1, spatial_grid(spatial_grid::MEDIUM));
                            transform::make_cell(p1, spatial_grid(spatial_grid::HIGH));
                        }}
                    }
                    if (1) {
                        static const spatial_point test[] = { // latitude, longitude
                            { 48.7139, 44.4984 },   // cell_id = 156-163-67-177-4
                            { 55.7975, 49.2194 },   // cell_id = 157-178-149-55-4
                            { 47.2629, 39.7111 },   // cell_id = 163-78-72-221-4
                            { 47.261, 39.7068 },    // cell_id = 163-78-72-223-4
                            { 55.7831, 37.3567 },   // cell_id = 156-38-25-118-4
                            { 0, -86 },             // cell_id = 128-234-255-15-4
                            { 45, -135 },           // cell_id = 70-170-170-170-4 | 73-255-255-255-4 | 118-0-0-0-4 | 121-85-85-85-4 
                            { 45, 135 },            // cell_id = 91-255-255-255-4 | 92-170-170-170-4 | 99-85-85-85-4 | 100-0-0-0-4
                            { 45, 0 },              // cell_id = 160-236-255-239-4 | 181-153-170-154-4
                            { 45, -45 },            // cell_id = 134-170-170-170-4 | 137-255-255-255-4 | 182-0-0-0-4 | 185-85-85-85-4
                            { 0, 0 },               // cell_id = 175-255-255-255-4 | 175-255-255-255-4
                            { 0, 135 },
                            { 0, 90 },
                            { 90, 0 },
                            { -90, 0 },
                            { 0, -45 },
                            { 45, 45 },
                            { 0, 180 },
                            { 0, -180 },
                            { 0, 131 },
                            { 0, 134 },
                            { 0, 144 },
                            { 0, 145 },
                            { 0, 166 },             // cell_id = 5-0-0-79-4 | 80-85-85-58-4
                        };
                        for (size_t i = 0; i < A_ARRAY_SIZE(test); ++i) {
                            //std::cout << i << ": " << to_string::type(test[i]) << " => ";
                            trace_cell(transform::make_cell(test[i], grid));
                        }
                    }
                    if (1) {
                        spatial_point p1 {};
                        spatial_point p2 {};
                        SDL_ASSERT(fequal(math::haversine(p1, p2), 0));
                        {
                            p2.latitude = 90.0 / 16;
                            const double h1 = math::haversine(p1, p2, limits::EARTH_RADIUS);
                            const double h2 = p2.latitude * limits::DEG_TO_RAD * limits::EARTH_RADIUS;
                            SDL_ASSERT(fequal(h1, h2));
                        }
                        {
                            p2.latitude = 90.0;
                            const double h1 = math::haversine(p1, p2, limits::EARTH_RADIUS);
                            const double h2 = p2.latitude * limits::DEG_TO_RAD * limits::EARTH_RADIUS;
                            SDL_ASSERT(fless(a_abs(h1 - h2), 1e-08));
                        }
                        if (math::EARTH_ELLIPSOUD) {
                            SDL_ASSERT(fequal(math::earth_radius(0), limits::EARTH_MAJOR_RADIUS));
                            SDL_ASSERT(fequal(math::earth_radius(90), limits::EARTH_MINOR_RADIUS));
                        }
                        else {
                            SDL_ASSERT(fequal(math::earth_radius(0), limits::EARTH_RADIUS));
                            SDL_ASSERT(fequal(math::earth_radius(90), limits::EARTH_RADIUS));
                        }
                    }
                }
                static void test_spatial() {
                    test_spatial(spatial_grid(spatial_grid::HIGH));
                }
                static void draw_grid() {
                    if (1) {
                        std::cout << "\ndraw_grid:\n";
                        const double sx = 16 * 4;
                        const double sy = 16 * 2;
                        const double dy = (SP::max_latitude - SP::min_latitude) / sy;
                        const double dx = (SP::max_longitude - SP::min_longitude) / sx;
                        size_t i = 0;
                        for (double y = SP::min_latitude; y <= SP::max_latitude; y += dy) {
                        for (double x = SP::min_longitude; x <= SP::max_longitude; x += dx) {
                            point_2D const p = math::project_globe(Latitude(y), Longitude(x));
                            std::cout << (i++) 
                                << "," << p.X
                                << "," << p.Y
                                << "," << x
                                << "," << y
                                << "\n";
                        }}
                    }
                    if (0) {
                        draw_circle(SP::init(Latitude(45), Longitude(0)), Meters(1000 * 1000));
                        draw_circle(SP::init(Latitude(0), Longitude(0)), Meters(1000 * 1000));
                        draw_circle(SP::init(Latitude(60), Longitude(45)), Meters(1000 * 1000));
                        draw_circle(SP::init(Latitude(85), Longitude(30)), Meters(1000 * 1000));
                        draw_circle(SP::init(Latitude(-60), Longitude(30)), Meters(1000 * 500));
                    }
                }
                static void draw_circle(SP const center, Meters const distance) {
                    //std::cout << "\ndraw_circle:\n";
                    const double bx = 1;
                    size_t i = 0;
                    for (double bearing = 0; bearing < 360; bearing += bx) {
                        SP const sp = math::destination(center, distance, Degree(bearing));
                        point_2D const p = math::project_globe(sp);
                        std::cout << (i++) 
                            << "," << p.X
                            << "," << p.Y
                            << "," << sp.longitude
                            << "," << sp.latitude
                            << "\n";
                    }
                };
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
