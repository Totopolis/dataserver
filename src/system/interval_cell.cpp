// interval_cell.cpp
//
#include "common/common.h"
#include "interval_cell.h"
#include "page_info.h"

namespace sdl { namespace db {

bool interval_cell::end_interval(iterator it) const {
    if (it != m_set.begin()) {
        return is_interval(*(--it));
    }
    return false;
}

bool interval_cell::insert_interval(uint32 const _32) {
    bool const ret = m_set.insert(spatial_cell::init(_32, zero_depth)).second;
    SDL_ASSERT(ret);
    return ret;
}

bool interval_cell::start_interval(iterator it) { // invalidates iterator
    auto const _32 = it->data.id._32;
    m_set.erase(it);
    return insert_interval(_32);
}

#if 0 //SDL_DEBUG
void interval_cell::trace() {
    SDL_TRACE("\ninterval_cell:");
    for (auto const & cell : m_set) {
        SDL_TRACE(to_string::type_less(cell));
    }
    SDL_TRACE("cell_count = ", cell_count());
}
bool interval_cell::insert(spatial_cell const & cell) {
    bool const b = _insert(cell);
    trace();
    return b;
}
bool interval_cell::_insert(spatial_cell const & cell) {
#else
bool interval_cell::insert(spatial_cell const & cell) { //FIXME: to be tested
#endif
    SDL_ASSERT(cell.data.depth == 4);
#if 0 //SDL_DEBUG
    static size_t call_cnt = 0;
    SDL_TRACE("\ninsert (", ++call_cnt, "): ", to_string::type_less(cell));
#endif
    iterator rh = m_set.lower_bound(cell);
    if (rh != m_set.end()) {
        if (is_same(*rh, cell)) {
            return false; // already exists
        }
        SDL_ASSERT(less()(cell, *rh));
        if (rh != m_set.begin()) {
            iterator lh = rh; --lh;
            SDL_ASSERT(less()(*lh, cell));
            if (is_interval(*lh)) { // insert in middle of interval
                SDL_ASSERT(!is_interval(*rh));
                return false;
            }
            if (is_next(*lh, cell)) {
                if (end_interval(lh)) {
                    m_set.erase(lh);
                }                
                else {
                    if (is_next(cell, *rh)) {
                        return start_interval(lh);
                    }
                    start_interval(lh);
                }
            }
            else if (is_next(cell, *rh)) {
                if (is_interval(*rh)) {
                    m_set.erase(rh);
                }
                return insert_interval(cell.data.id._32);
            }
        }
        else {
            if (is_next(cell, *rh)) { // merge interval
                if (is_interval(*rh)) {
                    m_set.erase(rh);
                }
                return insert_interval(cell.data.id._32);
            }
        }
    }
    else if (!m_set.empty()) {
        --rh;
        SDL_ASSERT(less()(*rh, cell));
        if (is_next(*rh, cell)) { // merge interval
            if (end_interval(rh)) {
                SDL_ASSERT(rh != m_set.begin());
                m_set.erase(rh);
            }
            else {
                start_interval(rh);
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
                    if (1) {
                        SDL_ASSERT(test.insert(spatial_cell::min()));
                        SDL_ASSERT(!test.insert(spatial_cell::min()));
                        SDL_ASSERT(test.insert(spatial_cell::max()));
                        c1 = spatial_cell::max();
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
                    //SDL_TRACE("\ninterval_cell tested");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
