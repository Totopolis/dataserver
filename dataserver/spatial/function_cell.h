// function_cell.h
//
#pragma once
#ifndef __SDL_SPATIAL_FUNCTION_CELL_H__
#define __SDL_SPATIAL_FUNCTION_CELL_H__

#include "dataserver/spatial/spatial_type.h"
#include "dataserver/common/algorithm.h"

namespace sdl { namespace db {

#if SDL_DEBUG
struct debug_function : is_static {
    static void trace(spatial_cell);
    static void trace(size_t const(&call_count)[spatial_cell::size]);
};
#endif

template<bool sort_cells>
class base_function_cell : noncopyable {
    SDL_DEBUG_HPP(size_t call_count[spatial_cell::size] = {};)
    enum { buf_size = spatial_grid::HIGH_HIGH };
    using vector_cell = vector_buf<spatial_cell, buf_size>;
    vector_cell buffer;
    virtual break_or_continue process(spatial_cell) = 0;
protected:
    base_function_cell() {
        buffer.reserve(buf_size);
    }
    virtual ~base_function_cell() {
        SDL_DEBUG_CPP(debug_function::trace(call_count));
        SDL_ASSERT(buffer.empty());
    }
public:
    break_or_continue operator()(spatial_cell const cell) {
        SDL_ASSERT(cell && cell.zero_tail());
        SDL_DEBUG_CPP(++call_count[cell.data.depth - 1]);
        if (buffer.size() < buf_size) {
            algo::unique_insertion(buffer, cell);
        }
        else {
            SDL_ASSERT(buffer.size() == buf_size);
            for (auto const & val : buffer) {
                SDL_DEBUG_CPP(debug_function::trace(val));
                if (is_break(process(val))) {
                    return bc::break_;
                }
            }
            buffer.clear();
        }
        return bc::continue_;
    }
    break_or_continue flush() {
        for (auto const & val : buffer) {
            SDL_DEBUG_CPP(debug_function::trace(val));
            if (is_break(process(val))) {
                return bc::break_;
            }
        }
        buffer.clear();
        return bc::continue_;
    }
};

template<>
class base_function_cell<false> {
    virtual break_or_continue process(spatial_cell) = 0;
protected:
    base_function_cell() = default;
    virtual ~base_function_cell() {}
public:
    break_or_continue operator()(spatial_cell const cell) {
        SDL_ASSERT(cell && cell.zero_tail());
        return process(cell);
    }
    static break_or_continue flush() {
        return bc::continue_;
    }
};

using function_cell = base_function_cell<false>;

template<class fun_type>
class function_cell_t : public function_cell {
    fun_type m_fun;
    break_or_continue process(spatial_cell cell) override {
        return make_break_or_continue(m_fun(cell));
    }
public:
    explicit function_cell_t(fun_type && f): m_fun(std::move(f)) {}
    ~function_cell_t(){}
};

} // db
} // sdl

#endif // __SDL_SPATIAL_FUNCTION_CELL_H__