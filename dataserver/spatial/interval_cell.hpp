// interval_cell.hpp
//
#pragma once
#ifndef __SDL_SYSTEM_INTERVAL_CELL_HPP__
#define __SDL_SYSTEM_INTERVAL_CELL_HPP__

namespace sdl { namespace db {

template<class fun_type>
interval_cell::const_iterator_bc
interval_cell::for_interval(const_iterator it, fun_type && fun) const
{
    SDL_ASSERT(it != m_set->end());
    SDL_ASSERT(get_depth(*it) == spatial_cell::size);
    if (is_interval(*it)) {
        const uint32 x1 = (it++)->r32();
        SDL_ASSERT(!is_interval(*it));
        SDL_ASSERT(it != m_set->end());
        const uint32 x2 = (it++)->r32();
#if 1 //FIXME: merge cells, prototype
        static_assert(cell_capacity<4>::upper_bound == 0xFF, "");
        static_assert(cell_capacity<4>::value == 0x100, "");
        if (x2 >= x1 + 0xFF) {
            const uint32 x11 = (x1 & ~uint32(0xFF)) + 0x100;
            if (x2 >= x11 + 0xFF) {
                const uint32 count11 = uint64(x2 - x11 + 1) / 0x100;
                const uint32 x22 = x11 + count11 * 0x100 - 1;
                SDL_ASSERT(x1 <= x11);
                SDL_ASSERT(x11 < x22);
                SDL_ASSERT(x22 <= x2);
                for (uint32 x = x1; x < x11; ++x) {
                    if (make_break_or_continue(fun(
                        spatial_cell::init(reverse_bytes(x), 4))) == bc::break_) {
                        return { it, bc::break_ }; 
                    }
                }
                if (x22 >= x11 + 0xFFFF) {
                    const uint32 x111 = (x11 & ~uint32(0xFFFF)) + 0x10000;
                    if (x22 >= x111 + 0xFFFF) {
                        const uint32 count111 = uint64(x22 - x111 + 1) / 0x10000;
                        const uint32 x222 = x111 + count111 * 0x10000 - 1;
                        SDL_ASSERT(x11 <= x111);
                        SDL_ASSERT(x111 < x222);
                        SDL_ASSERT(x222 <= x22);
                        for (uint32 x = x11; x < x111; x += 0x100) {
                            SDL_ASSERT(!(x & 0xFF));
                            if (make_break_or_continue(fun(
                                spatial_cell::init(reverse_bytes(x), 3))) == bc::break_) {
                                return { it, bc::break_ }; 
                            }
                        }
                        for (uint32 x = x111; x < x222; x += 0x10000) {
                            SDL_ASSERT(!(x & 0xFFFF));
                            if (make_break_or_continue(fun(
                                spatial_cell::init(reverse_bytes(x), 2))) == bc::break_) {
                                return { it, bc::break_ }; 
                            }
                        }
                        for (uint32 x = x222 + 1; x < x22; x += 0x100) {
                            SDL_ASSERT(!(x & 0xFF));
                            if (make_break_or_continue(fun(
                                spatial_cell::init(reverse_bytes(x), 3))) == bc::break_) {
                                return { it, bc::break_ }; 
                            }
                        }
                        goto continue_;
                    }
                }
                for (uint32 x = x11; x < x22; x += 0x100) {
                    SDL_ASSERT(!(x & 0xFF));
                    if (make_break_or_continue(fun(
                        spatial_cell::init(reverse_bytes(x), 3))) == bc::break_) {
                        return { it, bc::break_ }; 
                    }
                }
continue_:
                for (uint32 x = x22 + 1; x <= x2; ++x) {
                    if (make_break_or_continue(fun(
                        spatial_cell::init(reverse_bytes(x), 4))) == bc::break_) {
                        return { it, bc::break_ }; 
                    }
                }
                return { it, bc::continue_ };
            }
        }
#endif
        for (uint32 x = x1; x <= x2; ++x) {
            if (make_break_or_continue(fun(
                spatial_cell::init(reverse_bytes(x), spatial_cell::size))) == bc::break_) {
                return { it, bc::break_ }; 
            }
        }
        return { it, bc::continue_ };
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
        auto const p = for_interval(it, std::forward<fun_type>(fun));
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