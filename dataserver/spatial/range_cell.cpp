// range_cell.cpp
//
#include "dataserver/spatial/range_cell.h"

namespace sdl { namespace db {

range_cell::iterator
range_cell::begin() const {
    auto it = m_data.begin();
    const auto last = m_data.end();
    while (it != last) {
        if (const spatial_cell cell = (*it)()) {
            return iterator(this, state_type(cell, it));
        }
        ++it;
    }
    return this->end();
}

range_cell::iterator
range_cell::end() const {
    return iterator(this, state_type(spatial_cell(), m_data.end()));
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
                    range_cell tt;
                    tt.push_back([](){
                        return spatial_cell{};
                    });
                    for (spatial_cell cell : tt) {
                        SDL_ASSERT(cell);
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

#if 0
    class make_cell_depth_4 : noncopyable {
        using scan_lines_int = math::scan_lines_int;
        const rect_XY bbox;
        const scan_lines_int scan_lines;
    public:
        make_cell_depth_4(
            rect_XY const & bb,
            scan_lines_int && ll)
            : bbox(bb)
            , scan_lines(std::move(ll))
        {
            SDL_ASSERT(bbox.is_valid());
        }
        spatial_cell operator()() const {
            return{};
        }
    };
    static void test() {
        range_cell tt;
        auto p = std::make_shared<make_cell_depth_4>(rect_XY(), math::scan_lines_int());
        tt.push_back([p](){
            return (*p)();
        });
        for (spatial_cell cell : tt) {
            SDL_ASSERT(cell);
        }
    }
#endif