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

#if SDL_DEBUG
void interval_cell::insert(spatial_cell const & cell) {
    size_t const old = cell_count();
    const bool found = m_set.find(cell) != m_set.end();
    _insert(cell);
    trace(0);
    SDL_ASSERT(cell_count() == (found ? old : (old + 1)));
}
void interval_cell::_insert(spatial_cell const & cell) {
#else
void interval_cell::insert(spatial_cell const & cell) {
#endif
    SDL_ASSERT(cell.data.depth == 4);
    iterator const rh = m_set.lower_bound(cell);
    if (rh != m_set.end()) {
        if (is_same(*rh, cell)) {
            return; // already exists
        }
        SDL_ASSERT(is_less(cell, *rh));
        if (rh != m_set.begin()) { // insert in middle of set
            iterator lh = rh; --lh;
            SDL_ASSERT(is_less(*lh, cell));
            if (is_interval(*lh)) {
                SDL_ASSERT(!is_interval(*rh));
                return; // cell is inside interval [lh..rh]
            }
            if (is_next(*lh, cell)) {
                if (end_interval(lh)) {
                    SDL_ASSERT(!is_next(*lh, *rh));
                    if (is_next(cell, *rh)) {
                        m_set.erase(lh);
                        if (is_interval(*rh)) { // merge two intervals
                            m_set.erase(rh);
                        }
                    }
                    else {
                        m_set.insert(m_set.erase(lh), cell);
                    }
                    return;
                }                
                start_interval(lh);
                if (is_next(cell, *rh)) { // merge two intervals
                    if (is_interval(*rh)) {
                        m_set.erase(rh);
                    }
                    return;
                }
            }
            else if (is_next(cell, *rh)) {
                if (is_interval(*rh))
                    m_set.insert(m_set.erase(rh), cell);
                else
                    insert_interval(rh, cell);
                return;
            }
        }
        else { // insert at begin of set
            SDL_ASSERT(rh == m_set.begin());
            if (is_next(cell, *rh)) { // merge interval
                if (is_interval(*rh))
                    insert_interval(m_set.erase(rh), cell);
                else
                    insert_interval(rh, cell);
                return;
            }
        }
    }
    else if (!m_set.empty()) { // insert at end of set
        SDL_ASSERT(rh == m_set.end());
        iterator lh = rh; --lh;
        SDL_ASSERT(is_less(*lh, cell));
        if (is_next(*lh, cell)) { // merge interval
            if (end_interval(lh)) {
                m_set.insert(m_set.erase(lh), cell);
                return;
            }
            else {
                start_interval(lh);
            }
        }
    }
    m_set.insert(rh, cell); //use iterator hint when possible
}

size_t interval_cell::cell_count() const 
{
    size_t count = 0;
    auto const last = m_set.end();
    bool interval = false;
    uint32 start = uint32(-1);
    for (auto it = m_set.begin(); it != last; ++it) {
#if SDL_DEBUG
        {
            if (it != m_set.begin()) {
                auto prev = it; --prev;
                if (is_next(*prev, *it)) {
                    SDL_ASSERT(is_interval(*prev));
                    SDL_ASSERT(!is_interval(*it));
                }
            }
        }
#endif
        if (is_interval(*it)) {
            SDL_ASSERT(!interval);
            interval = true;
            start = it->r32();
        }
        else {
            if (interval) {
                interval = false;
                SDL_ASSERT(it->r32() > start);
                count += it->r32() - start + 1;
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
                        SDL_ASSERT(interval_cell::is_less(c1, c2));
                        test.insert(c1);
                        test.insert(c2);
                        c2[3] = 2; test.insert(c2);
                        c1[3] = 3; test.insert(c1);
                        for (size_t i = 0; i < 5; ++i) {
                            c1.data.id._32 = reverse_bytes(c1.data.id.r32() + 1);
                            test.insert(c1);
                        }
                    }
                    if (1) {
                        spatial_cell c1;
                        c1.data.depth = 4;
                        c1[0] = 5;
                        c1[1] = 6;
                        c1[2] = 7;
                        c1[3] = 8;     test.insert(c1);
                        c1[3] = 7;     test.insert(c1);
                        c1[3] = 12;    test.insert(c1);
                        c1[3] = 10;    test.insert(c1);
                        c1[3] = 11;    test.insert(c1);
                        c1[3] = 9;     test.insert(c1);
                        c1[3] = 13;    test.insert(c1);
                        c1[3] = 16;    test.insert(c1);
                        c1[3] = 17;    test.insert(c1); 
                    }
                    if (1) {
                        spatial_cell c1;
                        c1.data.depth = 4;
                        c1[0] = 1;
                        c1[1] = 2;
                        c1[2] = 3;
                        c1[3] = 2;
                        test.insert(c1); 
                        c1[3] = 1;
                        test.insert(c1); 
                    }
                    if (1) {
                        test.insert(spatial_cell::min());
                        test.insert(spatial_cell::min());
                        test.insert(spatial_cell::max());
                        spatial_cell c1 = spatial_cell::max();
                        c1[3] = 0;
                        test.insert(c1);
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
                        SDL_ASSERT(!test.empty());
                        interval_cell test2 = std::move(test);
                        SDL_ASSERT(!test2.empty());
                        SDL_ASSERT(test.empty());
                        test.swap(test2);
                        SDL_ASSERT(!test.empty());
                    }
                    test.clear();
                    if (1) {
                        interval_cell test2;
                        std::set<uint32> unique;
                        size_t count = 0;
                        for (int i = 0; i < 100000; ++i) {
                            const uint32 v = rand();
                            if (unique.insert(v).second) {
                                ++count;
                            }
                            test2._insert(spatial_cell::init(v));
                        }
                        SDL_ASSERT(count == unique.size());
                        SDL_ASSERT(count == test2.cell_count());
                        test2.for_each([&unique, &count](spatial_cell const & x){
                            size_t const n = unique.erase(x.data.id._32);
                            SDL_ASSERT(n == 1);
                            --count;
                            return bc::continue_;
                        });
                        SDL_ASSERT(count == 0);
                        SDL_ASSERT(unique.empty());
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG
