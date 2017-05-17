// interval_cell.cpp
//
#include "dataserver/spatial/interval_cell.h"
#include "dataserver/system/page_info.h"

#if SDL_USE_INTERVAL_CELL || defined(SDL_OS_WIN32)

#if SDL_DEBUG > 1
#include "dataserver/spatial/transform.h"
#include <iomanip> // for std::setprecision
#endif

#if 0
//FIXME: C++ containers that save memory and time
https://code.google.com/archive/p/cpp-btree/
http://google-opensource.blogspot.ru/2013/01/c-containers-that-save-memory-and-time.html?m=1
#endif

namespace sdl { namespace db {

#if SDL_DEBUG > 1
void interval_cell::trace(bool const enabled) {
    if (enabled) {
        SDL_TRACE("\ninterval_cell:");
        for (auto const & x : *m_set) {
            SDL_TRACE(to_string::type_less(x), ", _32 = ", x.data.id._32);
        }
        SDL_TRACE("cell_count = ", this->size());
    }
}
namespace { 
    void debug_trace(spatial_cell const cell) {
        point_2D const p = transform::cell2point(cell);
        spatial_point const sp = transform::spatial(cell);
        static int i = 0;
        std::cout << (i++)
            << std::setprecision(9)
            << "," << p.X
            << "," << p.Y
            << "," << sp.longitude
            << "," << sp.latitude
            << "\n";
    }
}
#endif

void interval_cell::insert(spatial_cell const cell) {
    SDL_ASSERT(cell.data.depth == spatial_cell::depth_4);
    SDL_ASSERT(cell.zero_tail());
    set_type & this_set = *m_set;
    iterator const rh = this_set.lower_bound(cell);
    if (rh != this_set.end()) {
        if (is_same(*rh, cell)) {
            return; // already exists
        }
        SDL_ASSERT(is_less(cell, *rh));
        if (rh != this_set.begin()) { // insert in middle of set
            iterator const lh = previous_iterator(rh);
            SDL_ASSERT(is_less(*lh, cell));
            if (is_interval(*lh)) {
                SDL_ASSERT(!is_interval(*rh));
                return; // cell is inside interval [lh..rh]
            }
            if (is_next(*lh, cell)) {
                if (end_interval(lh)) {
                    SDL_ASSERT(!is_next(*lh, *rh));
                    if (is_next(cell, *rh)) { // merge [..lh][cell][rh..]
                        if (is_interval(*rh)) { // merge two intervals
                            auto last = rh; ++last;
                            this_set.erase(lh, last);
                        }
                        else {
                            this_set.erase(lh);
                        }
                    }
                    else { // merge [..lh][cell]
                        this_set.insert(this_set.erase(lh), cell);
                    }
                    return;
                }
                start_interval(lh);
                if (is_next(cell, *rh)) { // merge two intervals
                    if (is_interval(*rh)) {
                        this_set.erase(rh);
                    }
                    return;
                }
                this_set.insert(rh, cell);
                return;
            }
        }
        SDL_ASSERT((rh == this_set.begin()) || !is_next(*previous_iterator(rh), cell));
        if (is_next(cell, *rh)) { // merge [cell][rh..]
            if (is_interval(*rh))
                insert_interval(this_set.erase(rh), cell);
            else
                insert_interval(rh, cell);
            return;
        }
    }
    else if (!this_set.empty()) { // insert at end of set
        SDL_ASSERT(rh == this_set.end());
        iterator const lh = previous_iterator(rh);
        SDL_ASSERT(is_less(*lh, cell));
        if (is_next(*lh, cell)) { // merge interval
            if (end_interval(lh)) {
                this_set.insert(this_set.erase(lh), cell);
                return;
            }
            start_interval(lh);
        }
    }
    this_set.insert(rh, cell); //use iterator hint when possible
}

void interval_cell::insert_range(spatial_cell const c1, spatial_cell const c2) // to be tested
{
    SDL_ASSERT(c1.data.depth == spatial_cell::depth_4);
    SDL_ASSERT(c2.data.depth == spatial_cell::depth_4);
    SDL_ASSERT(is_less(c1, c2));
    set_type & this_set = *m_set;
    if (this_set.empty()) {
        insert_interval(c1, c2);
        return;
    }
    else {
        iterator const rh = this_set.lower_bound(c1);
        if (rh == this_set.end()) { // insert at end of set
            SDL_ASSERT(!is_find(c1));
            SDL_ASSERT(!is_find(c2));
            iterator const lh = previous_iterator(rh);
            SDL_ASSERT(is_less(*lh, c1));
            if (is_next(*lh, c1)) { // merge [..lh][c1..c2]
                if (end_interval(lh)) {
                    insert_interval(this_set.erase(lh), c1, c2);
                    return;
                }
                start_interval(lh);
                this_set.insert(rh, c2);
                return;
            }
            insert_interval(rh, c1, c2);
            return;
        }
        else {
            if (is_same(*rh, c1)) {
                if (is_interval(*rh)) { // merge [rh..] and [rh..c2]
                    iterator const next = next_iterator(rh);
                    SDL_ASSERT(next != m_set->end());
                    if (is_same(*next, c2)) {
                        return; // interval already exists
                    }
                    if (is_less(c2, *next)) { // [rh[c1..c2]..next]
                        return;
                    }
                    SDL_ASSERT(is_less(*next, c2));
                    iterator const rh2 = this_set.lower_bound(c2);
                    SDL_ASSERT(rh2 != next);
                    if (rh2 == this_set.end()) { // [c1[rh..next]..c2]rh2=end
                        m_set->insert(this_set.erase(next, rh2), c2);
                        return;
                    }
                    if (is_same(*rh2, c2) || is_next(c2, *rh2)) { // [rh|c1..next..c2][rh2..]
                        this_set.erase(next, is_interval(*rh2) ? next_iterator(rh2) : rh2);
                        return;
                    }
                    m_set->insert(this_set.erase(next, rh2), c2);
                    return;
                }
                else { // merge [..rh][c1..c2]
                    SDL_ASSERT(!is_interval(*rh));
                    if (end_interval(rh)) {
                        SDL_ASSERT(is_same(*rh, c1));
                        iterator const rh2 = this_set.lower_bound(c2);
                        if (rh2 == this_set.end()) { // [..rh][c1..c2]rh2=end
                            this_set.insert(this_set.erase(rh, rh2), c2);
                            return;
                        }
                        if (is_same(*rh2, c2) || is_next(c2, *rh2)) { // [..rh][c1..c2][rh2..]
                            this_set.erase(rh, is_interval(*rh2) ? next_iterator(rh2) : rh2);
                            return;
                        }
                        SDL_ASSERT(is_less(c2, *rh2));
                        this_set.insert(this_set.erase(rh, rh2), c2);
                        return;
                    }
                    else {
                        SDL_ASSERT(!end_interval(rh) && is_same(*rh, c1));
                        start_interval(rh);
                        iterator const rh2 = this_set.lower_bound(c2);
                        if (rh2 == this_set.end()) {  // [rh][c1..c2]rh2=end
                            this_set.insert(this_set.erase(next_iterator(rh), rh2), c2);
                            return;
                        }
                        if (is_same(*rh2, c2) || is_next(c2, *rh2)) { // [c1..c2][rh2..]
                            this_set.erase(next_iterator(rh), is_interval(*rh2) ? next_iterator(rh2) : rh2);
                            return;
                        }
                        SDL_ASSERT(is_less(c2, *rh2));
                        this_set.insert(this_set.erase(next_iterator(rh), rh2), c2);
                        return;
                    }
                }
                SDL_ASSERT(0);
            }
            SDL_ASSERT(is_less(c1, *rh)); // !is_same(*rh, c1)
            iterator const rh2 = this_set.lower_bound(c2);
            if (rh2 == this_set.end()) {
                iterator const lh = previous_iterator(rh);
                SDL_ASSERT(is_less(*lh, c1));
                if (is_next(*lh, c1)) { // [..lh][c1..rh..c2]rh2=end
                    if (end_interval(lh)) {
                        this_set.insert(this_set.erase(lh, rh2), c2);
                        return;
                    }
                    start_interval(lh);
                    this_set.insert(this_set.erase(rh, rh2), c2);
                    return;
                }
                insert_interval(this_set.erase(rh, rh2), c1, c2); 
                return;
            }
            if (rh2 == this_set.begin()) { // [c1..c2][rh2..]
                SDL_ASSERT(rh == rh2);
                if (is_same(*rh2, c2) || is_next(c2, *rh2)) { // [c1..c2][rh2..]
                    if (is_interval(*rh2)) {
                        insert_interval(this_set.erase(rh2), c1);
                        return;
                    }
                    insert_interval(rh2, c1);
                    return;
                }
                SDL_ASSERT(is_less(c2, *rh2));
                insert_interval(rh2, c1, c2);
                return;
            }
            if (rh == this_set.begin()) { // [c1..begin..c2..rh2][rh2..]
                if (is_same(*rh2, c2) || is_next(c2, *rh2)) {
                    insert_interval(this_set.erase(rh, is_interval(*rh2) ? next_iterator(rh2) : rh2), c1);
                    return;
                }
                insert_interval(this_set.erase(rh, rh2), c1, c2);
                return;
            }
            SDL_ASSERT(!is_same(*rh, c1));
            SDL_ASSERT(rh != this_set.end());
            SDL_ASSERT(rh != this_set.begin());
            SDL_ASSERT(rh2 != this_set.begin());
            SDL_ASSERT(rh2 != this_set.end());
            if (is_same(*rh2, c2) || is_next(c2, *rh2)) {
                if (end_interval(rh)) {
                    this_set.erase(rh, is_interval(*rh2) ? next_iterator(rh2) : rh2);
                    return;
                }
                iterator const lh = previous_iterator(rh);
                if (is_next(*lh, c1)) {
                    SDL_ASSERT(!is_interval(*lh));
                    if (end_interval(lh)) {
                        this_set.erase(lh, is_interval(*rh2) ? next_iterator(rh2) : rh2);
                    }
                    else {
                        start_interval(lh);
                        this_set.erase(rh, is_interval(*rh2) ? next_iterator(rh2) : rh2);
                    }
                    return;
                }
                insert_interval(this_set.erase(rh, is_interval(*rh2) ? next_iterator(rh2) : rh2), c1);
                return;
            }
            else {
                SDL_ASSERT(is_less(c1, *rh));
                SDL_ASSERT(is_less(c2, *rh2));
                SDL_ASSERT(!is_next(c2, *rh2));
                iterator const lh = previous_iterator(rh);
                if (is_interval(*lh)) {
                    this_set.insert(this_set.erase(rh, rh2), c2);
                    return;
                }
                SDL_ASSERT(!end_interval(rh));
                if (is_next(*lh, c1)) {
                    if (end_interval(lh)) {
                        this_set.insert(this_set.erase(lh, rh2), c2);
                        return;
                    }
                    start_interval(lh);
                    this_set.insert(this_set.erase(rh, rh2), c2);
                    return;
                }
                if (rh != rh2) {
                    insert_interval(this_set.erase(rh, rh2), c1, c2);
                    return;
                }
                insert_interval(rh2, c1, c2);
                return;
            }
        }
    }
}

void interval_cell::insert_depth_1(spatial_cell const cell)
{
    SDL_ASSERT(cell.data.depth == spatial_cell::depth_1);
    SDL_ASSERT(cell.zero_tail());
    spatial_cell c1 = cell;
    spatial_cell c2 = cell; 
    c1.data.depth = spatial_cell::depth_4;
    c2.data.depth = spatial_cell::depth_4;
    c2.set_id<1>(uint8(0xFF));
    c2.set_id<2>(uint8(0xFF));
    c2.set_id<3>(uint8(0xFF));
    insert_range(c1, c2);
    //FIXME:SDL_ASSERT_DEBUG_2(size());
}

void interval_cell::insert_depth_2(spatial_cell const cell)
{
    SDL_ASSERT(cell.data.depth == spatial_cell::depth_2);
    SDL_ASSERT(cell.zero_tail());
    spatial_cell c1 = cell;
    spatial_cell c2 = cell; 
    c1.data.depth = spatial_cell::depth_4;
    c2.data.depth = spatial_cell::depth_4;
    c2.set_id<2>(uint8(0xFF));
    c2.set_id<3>(uint8(0xFF));
    insert_range(c1, c2);
    //FIXME:SDL_ASSERT_DEBUG_2(size());
}

void interval_cell::insert_depth_3(spatial_cell const cell)
{
    SDL_ASSERT(cell.data.depth == spatial_cell::depth_3);
    SDL_ASSERT(cell.zero_tail());
    spatial_cell c1 = cell;
    spatial_cell c2 = cell; 
    c1.data.depth = spatial_cell::depth_4;
    c2.data.depth = spatial_cell::depth_4;
    c2.set_id<3>(uint8(0xFF));
    insert_range(c1, c2);
    //FIXME:SDL_ASSERT_DEBUG_2(size());
}

bool interval_cell::find(spatial_cell const cell) const 
{
    SDL_ASSERT(cell.data.depth == spatial_cell::size);
    SDL_ASSERT(cell.zero_tail());
    auto rh = m_set->lower_bound(cell);
    if (rh != m_set->end()) {
        if (is_same(*rh, cell)) {
            return true;
        }
        if (rh != m_set->begin()) {
            return is_interval(*(--rh));
        }
    }
    return false;
}

size_t interval_cell::size() const 
{
    size_t count = 0;
    auto const last = m_set->end();
    bool interval = false;
    uint32 start = 0;
    for (auto it = m_set->begin(); it != last; ++it) {
#if SDL_DEBUG > 1
        if (it != m_set->begin()) {
            auto prev = it; --prev;
            if (is_next(*prev, *it)) {
                SDL_ASSERT(is_interval(*prev));
                SDL_ASSERT(!is_interval(*it));
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
                        //SDL_ASSERT(interval_cell::is_less(c1, c2));
                        test.insert(c1);
                        c1[3] = 5;
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
                        SDL_ASSERT(count_cell == test.size());
                        SDL_ASSERT(res == bc::continue_);
                        SDL_ASSERT(!test.empty());
                        interval_cell test2;
                        test.swap(test2);
                        SDL_ASSERT(!test2.empty());
                    }
                    test.clear();
                    if (SDL_DEBUG > 2) {
                        const size_t max_try = 3;
                        const size_t max_i[max_try] = {5000, 100000, 700000};
                        for (size_t j = 0; j < max_try; ++j) {
                            SDL_TRACE("\ninterval_cell random test (", max_i[j], ")");
                            interval_cell test2;
                            std::set<spatial_cell> unique;
                            size_t count = 0;
                            for (size_t i = 0; i < max_i[j]; ++i) {
                                const double r1 = double(rand()) / RAND_MAX;
                                const double r2 = double(rand()) / RAND_MAX;
                                const uint32 _32 = static_cast<uint32>(uint32(-1) * r1 * r2);
                                spatial_cell const cell = spatial_cell::init(_32, spatial_cell::size);
                                if (unique.insert(cell).second) {
                                    ++count;
                                }
                                test2.insert(cell);
                                SDL_ASSERT(test2.find(cell));
                            }
                            SDL_ASSERT(count == unique.size());
                            size_t const test_count = test2.size();
                            SDL_ASSERT(count == test_count);
                            test2.for_each([&unique, &count](spatial_cell const & x){
                                SDL_ASSERT(x.data.depth == 4);
                                size_t const n = unique.erase(x);
                                SDL_ASSERT(n == 1);
                                --count;
                                return bc::continue_;
                            });
                            SDL_ASSERT(count == 0);
                            SDL_ASSERT(unique.empty());
                            {
                                size_t count_cell = 0;
                                size_t count_intv = 0;
                                test2.for_each_interval(
                                    [&count_cell](spatial_cell const &){
                                    ++count_cell;
                                    return bc::continue_;
                                },
                                    [&count_intv](spatial_cell const &, spatial_cell const &){
                                    ++count_intv;
                                    return bc::continue_;
                                });
                                SDL_TRACE("cell = ", count_cell, ", intv = ", count_intv);
                            }
                        }
                    }
                    using namespace interval_cell_;
                    static_assert(align_cell<spatial_cell::depth_4>(0xFF) == 0x100, "");
                    static_assert(align_cell<spatial_cell::depth_3>(0xFF) == 0x10000, "");
                    static_assert(align_cell<spatial_cell::depth_2>(0xFF) == 0x1000000, "");
                    static_assert(is_align_cell<spatial_cell::depth_4>(align_cell<spatial_cell::depth_4>(0xFF)), "");
                    static_assert(is_align_cell<spatial_cell::depth_3>(align_cell<spatial_cell::depth_3>(0xFF)), "");
                    static_assert(is_align_cell<spatial_cell::depth_2>(align_cell<spatial_cell::depth_2>(0xFF)), "");
                    SDL_ASSERT(upper_cell<spatial_cell::depth_4>(0x100, 0x1000000) == 0xFFFFFF);
                    SDL_ASSERT(upper_cell<spatial_cell::depth_4>(0x100, 0xFFFFFFFF) == 0xFFFFFFFF);
                    static_assert(cell_capacity<spatial_cell::depth_4>::value == 256, "");
                    static_assert(cell_capacity<spatial_cell::depth_3>::value == 256 * 256, "");
                    static_assert(cell_capacity<spatial_cell::depth_2>::value == 256 * 256 * 256, "");
                    static_assert(cell_capacity<spatial_cell::depth_1>::value64 == uint64(256) * 256 * 256 * 256, "");
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SDL_DEBUG
#endif // #if SDL_USE_INTERVAL_CELL
