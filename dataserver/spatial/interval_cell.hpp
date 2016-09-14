// interval_cell.hpp
//
#pragma once
#ifndef __SDL_SYSTEM_INTERVAL_CELL_HPP__
#define __SDL_SYSTEM_INTERVAL_CELL_HPP__

namespace sdl { namespace db {

template<interval_cell::depth_t depth, class fun_type> break_or_continue
interval_cell::for_range(uint32 x1, uint32 const x2, fun_type && fun) {
    SDL_ASSERT(x1 <= x2);
    while (x1 < x2) {
        SDL_ASSERT((depth == 4) || !(x1 & (cell_capacity<depth>::step - 1)));
        if (make_break_or_continue(fun(
            spatial_cell::init(reverse_bytes(x1), depth))) == bc::break_) {
            return bc::break_; 
        }
        x1 += cell_capacity<depth>::step;
    }
    SDL_ASSERT(x1 == x2);
    return bc::continue_;
}

template<interval_cell::depth_t depth, class fun_type> break_or_continue 
interval_cell::merge_cells(uint32 const x1, uint32 const x2, fun_type && fun)
{
    return bc::continue_;
}

template<class fun_type> 
interval_cell::const_iterator_bc
interval_cell::for_interval(const_iterator it, fun_type && fun) const
{
    static_assert(cell_capacity<4>::upper == 0xFF, "");
    static_assert(cell_capacity<3>::upper == 0xFFFF, "");
    static_assert(cell_capacity<2>::upper == 0xFFFFFF, "");
    static_assert(cell_capacity<1>::upper == 0xFFFFFFFF, "");
    static_assert(cell_capacity<4>::value == 0x100, "");
    static_assert(cell_capacity<3>::value == 0x10000, "");
    static_assert(cell_capacity<2>::value == 0x1000000, "");
    static_assert(cell_capacity<1>::value64 == 0x100000000, "");

    SDL_ASSERT(it != m_set->end());
    SDL_ASSERT(get_depth(*it) == spatial_cell::size);
    if (is_interval(*it)) {
        const uint32 x1 = (it++)->r32();
        SDL_ASSERT(!is_interval(*it));
        SDL_ASSERT(it != m_set->end());
        const uint32 x2 = (it++)->r32();
#if 1 //FIXME: merge cells, prototype
        if (x2 >= x1 + cell_capacity<4>::upper) {
            const uint32 x11 = (x1 & ~uint32(cell_capacity<4>::upper)) + cell_capacity<4>::value;
            if (x2 >= x11 + cell_capacity<4>::upper) {
                const uint32 count11 = (x2 - x11 + 1) / cell_capacity<4>::value;
                const uint32 x22 = x11 + count11 * cell_capacity<4>::value - 1;
                SDL_ASSERT(x1 <= x11);
                SDL_ASSERT(x11 < x22);
                SDL_ASSERT(x22 <= x2);                
                if (for_range<spatial_cell::depth_4>(x1, x11, fun) == bc::break_) {
                    return { it, bc::break_ }; 
                }
                if (x22 >= x11 + cell_capacity<3>::upper) {
                    const uint32 x111 = (x11 & ~uint32(cell_capacity<3>::upper)) + cell_capacity<3>::value;
                    if (x22 >= x111 + cell_capacity<3>::upper) {
                        const uint32 count111 = (x22 - x111 + 1) / cell_capacity<3>::value;
                        const uint32 x222 = x111 + count111 * cell_capacity<3>::value - 1;
                        SDL_ASSERT(x11 <= x111);
                        SDL_ASSERT(x111 < x222);
                        SDL_ASSERT(x222 <= x22);
                        if (for_range<spatial_cell::depth_3>(x11, x111, fun) == bc::break_)         return { it, bc::break_ };
                        if (for_range<spatial_cell::depth_2>(x111, x222 + 1, fun) == bc::break_)    return { it, bc::break_ };
                        if (for_range<spatial_cell::depth_3>(x222 + 1, x22 + 1, fun) == bc::break_) return { it, bc::break_ };
                        goto continue_; // return { it, for_range<spatial_cell::depth_4>(x22 + 1, x2 + 1, fun) };
                    }
                }
                if (for_range<spatial_cell::depth_3>(x11, x22 + 1, fun) == bc::break_) {
                    return { it, bc::break_ }; 
                }
            continue_:
                return { it, for_range<spatial_cell::depth_4>(x22 + 1, x2 + 1, fun) };
            }
        }
#endif
        return { it, for_range<spatial_cell::depth_4>(x1, x2 + 1, fun) };
    }
    else {
        SDL_ASSERT(it->data.depth == spatial_cell::size);
        break_or_continue const b = make_break_or_continue(fun(*it)); 
        return { ++it, b };
    }
}

template<class fun_type>
break_or_continue interval_cell::for_each(fun_type && fun) const
{
    auto const last = m_set->end();
    auto it = m_set->begin();
    while (it != last) {
        auto const p = for_interval(it, fun);
        if (p.second == bc::break_) {
            return bc::break_;
        }
        it = p.first;
    }
    return bc::continue_;
}

template<class cell_fun, class interval_fun>
break_or_continue interval_cell::for_each_interval(cell_fun && fun1, interval_fun && fun2) const
{
    auto const last = m_set->end();
    auto it = m_set->begin();
    while (it != last) {
        SDL_ASSERT(get_depth(*it) == spatial_cell::size);
        if (is_interval(*it)) {
            const uint32 x1 = (it++)->r32();
            SDL_ASSERT(!is_interval(*it));
            SDL_ASSERT(it != m_set->end());
            const uint32 x2 = (it++)->r32();
            SDL_ASSERT(x1 < x2);
            const spatial_cell c1 = spatial_cell::init(reverse_bytes(x1), spatial_cell::size);
            const spatial_cell c2 = spatial_cell::init(reverse_bytes(x2), spatial_cell::size);
            SDL_ASSERT(c1 < c2);
            if (make_break_or_continue(fun2(c1, c2)) == bc::break_) {
                return bc::break_;
            }
        }
        else {
            SDL_ASSERT(it->data.depth == spatial_cell::size);
            if (make_break_or_continue(fun1(*it++)) == bc::break_) {
                return bc::break_;
            }
        }
    }
    return bc::continue_;
}

} // db
} // sdl

#endif // __SDL_SYSTEM_INTERVAL_CELL_HPP__