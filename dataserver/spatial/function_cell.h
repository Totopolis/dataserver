// function_cell.h
//
#pragma once
#ifndef __SDL_SPATIAL_FUNCTION_CELL_H__
#define __SDL_SPATIAL_FUNCTION_CELL_H__

#include "dataserver/spatial/spatial_type.h"
#include "dataserver/common/algorithm.h"

namespace sdl { namespace db {

#define function_cell_sort_cells    1
#if function_cell_sort_cells    // to be tested
class function_cell : noncopyable {
    SDL_DEBUG_CODE(size_t call_count[spatial_cell::size] = {};)
    enum { buf_size = spatial_grid::HIGH_HIGH };
    using vector_cell = vector_buf<spatial_cell, buf_size>;
    vector_cell buffer;
    virtual break_or_continue process(spatial_cell) = 0;
    SDL_DEBUG_CODE(static void trace(spatial_cell);)
    SDL_DEBUG_CODE(void trace_call_count() const;)
protected:
    function_cell() {
        buffer.reserve(buf_size);
    }
    ~function_cell() {
        SDL_DEBUG_CODE(trace_call_count();)
        SDL_ASSERT(buffer.empty());
    }
public:
    break_or_continue operator()(spatial_cell const cell) {
        SDL_ASSERT(cell && cell.zero_tail());
        SDL_DEBUG_CODE(++call_count[cell.data.depth-1];)
        if (buffer.size() < buf_size) {
            algo::unique_insertion(buffer, cell);
        }
        else {
            SDL_ASSERT(buffer.size() == buf_size);
            for (auto const & val : buffer) {
                SDL_DEBUG_CODE(trace(val);)
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
            SDL_DEBUG_CODE(trace(val);)
            if (is_break(process(val))) {
                return bc::break_;
            }
        }
        buffer.clear();
        return bc::continue_;
    }
};
#else
class function_cell {
    SDL_DEBUG_CODE(size_t call_count[4] = {};)
    virtual break_or_continue process(spatial_cell) = 0;
    SDL_DEBUG_CODE(static void trace(spatial_cell);)
    SDL_DEBUG_CODE(void trace_call_count() const;)
protected:
    function_cell() {}
    ~function_cell() {
        SDL_DEBUG_CODE(trace_call_count();)
    }
public:
    break_or_continue operator()(spatial_cell const cell) {
        SDL_ASSERT(cell && cell.zero_tail());
        SDL_DEBUG_CODE(++call_count[cell.data.depth-1];)
        return process(cell);
    }
    static break_or_continue flush() {
        return bc::continue_;
    }
};
#endif // #if function_cell_sort_cells

template<class fun_type>
class function_cell_t : public function_cell {
    fun_type m_fun;
    break_or_continue process(spatial_cell cell) override {
        return m_fun(cell);
    }
public:
    explicit function_cell_t(fun_type && f): m_fun(std::move(f)) {}
};

} // db
} // sdl

#endif // __SDL_SPATIAL_FUNCTION_CELL_H__