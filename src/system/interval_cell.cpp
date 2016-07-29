// interval_cell.cpp
//
#include "common/common.h"
#include "interval_cell.h"
#include "page_info.h"

namespace sdl { namespace db {

#if SDL_DEBUG
void interval_cell::trace(bool const enabled) {
    if (enabled) {
        SDL_TRACE("\ninterval_cell:");
        for (auto const & cell : m_set) {
            SDL_TRACE(to_string::type_less(cell));
        }
        SDL_TRACE("cell_count = ", cell_count());
    }
}
#endif

bool interval_cell::insert(spatial_cell const & cell) // to be tested
{
    SDL_ASSERT(cell.data.depth == 4);
    iterator rh = m_set.lower_bound(cell);
    if (rh != m_set.end()) {
        if (is_same(*rh, cell)) {
            return false; // already exists
        }
        SDL_ASSERT(less()(cell, *rh));
        if (rh != m_set.begin()) { // insert in middle
            iterator lh = rh; --lh;
            SDL_ASSERT(less()(*lh, cell));
            if (is_interval(*lh)) {
                SDL_ASSERT(!is_interval(*rh));
                return false; // cell is inside the interval [lh..rh]
            }
            if (is_next(*lh, cell)) {
                if (end_interval(lh)) {
                    m_set.erase(lh);
                }                
                else {
                    start_interval(lh); // invalidates lh
                }
                if (is_next(cell, *rh)) {
                    if (is_interval(*rh)) {
                        m_set.erase(rh); // merged two intervals
                    } // else [..rh] is new end of interval
                    return false;
                }
            }
            else if (is_next(cell, *rh)) {
                if (is_interval(*rh)) {
                    m_set.erase(rh);
                }
                return insert_interval(cell);
            }
        }
        else { // rh == m_set.begin()
            if (is_next(cell, *rh)) { // merge interval
                if (is_interval(*rh)) {
                    m_set.erase(rh);
                }
                return insert_interval(cell);
            }
        }
    }
    else if (!m_set.empty()) { // rh == m_set.end()
        --rh;
        SDL_ASSERT(less()(*rh, cell));
        if (is_next(*rh, cell)) { // merge interval
            if (end_interval(rh)) {
                SDL_ASSERT(rh != m_set.begin());
                m_set.erase(rh);
            }
            else {
                start_interval(rh); // invalidates rh
            }
        }
    }
    return m_set.insert(cell).second;
}

size_t interval_cell::cell_count() const 
{
    size_t count = 0;
    auto const last = m_set.end();
    bool interval = false;
    uint32 start = uint32(-1);
    for (auto it = m_set.begin(); it != last; ++it) {
        if (is_interval(*it)) {
            SDL_ASSERT(!interval);
            interval = true;
            start = it->data.id.r32();
        }
        else {
            if (interval) {
                interval = false;
                SDL_ASSERT(it->data.id.r32() > start);
                count += it->data.id.r32() - start + 1;
            }
            else {
                ++count;
            }
        }
    }
    return count;
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
                    interval_cell test;
                    if (1) {
                        spatial_cell c1, c2;
                        c1.data.depth = 4;
                        c1[0] = 1;
                        c1[1] = 2;
                        c1[2] = 3;
                        c1[3] = 4;
                        c2.data.depth = 4;
                        c2[0] = 2;
                        c2[1] = 3;
                        c2[2] = 1;
                        c2[3] = 10;
                        SDL_ASSERT(c1 < c2);
                        SDL_ASSERT(interval_cell::less()(c1, c2));
                        SDL_ASSERT(test.insert(c1));
                        SDL_ASSERT(test.insert(c2));
                        c2[3] = 2; SDL_ASSERT(test.insert(c2));
                        c1[3] = 3; SDL_ASSERT(test.insert(c1));
                        for (size_t i = 0; i < 5; ++i) {
                            c1.data.id._32 = reverse_bytes(c1.data.id.r32() + 1);
                            test.insert(c1);
                        }
                    }
                    if (1) {
                        enum { trace = 0 };
                        spatial_cell c1;
                        c1.data.depth = 4;
                        c1[0] = 5;
                        c1[1] = 6;
                        c1[2] = 7;
                        c1[3] = 8;     test.insert(c1);
                        c1[3] = 7;     test.insert(c1);
                        c1[3] = 12;    test.insert(c1);  test.trace(trace);
                        c1[3] = 10;    test.insert(c1);  test.trace(trace);
                        c1[3] = 11;    test.insert(c1);  test.trace(trace);
                        c1[3] = 9;     test.insert(c1);  test.trace(trace);
                    }
                    if (1) {
                        SDL_ASSERT(test.insert(spatial_cell::min()));
                        SDL_ASSERT(!test.insert(spatial_cell::min()));
                        SDL_ASSERT(test.insert(spatial_cell::max()));
                        spatial_cell c1 = spatial_cell::max();
                        c1[3] = 0;
                        SDL_ASSERT(test.insert(c1));
                    }
                    if (1) {
                        spatial_cell old = spatial_cell::min();
                        size_t count_cell = 0;
                        break_or_continue res = test.for_each([&count_cell, &old](spatial_cell const & cell){
                            SDL_ASSERT(cell.data.depth == 4);
                            SDL_ASSERT(!(cell < old));
                            old = cell;
                            ++count_cell;
                            return bc::continue_;
                        });
                        SDL_ASSERT(count_cell);
                        SDL_ASSERT(count_cell == test.cell_count());
                        SDL_ASSERT(res == bc::continue_);
                    }
                    SDL_ASSERT(!test.empty());
                    interval_cell test2 = std::move(test);
                    SDL_ASSERT(!test2.empty());
                    SDL_ASSERT(test.empty());
                    test.swap(test2);
                    SDL_ASSERT(!test.empty());
                    //SDL_TRACE("\ninterval_cell tested");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
