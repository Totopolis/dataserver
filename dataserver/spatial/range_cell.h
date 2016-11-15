// range_cell.h
//
#pragma once
#ifndef __SDL_SPATIAL_RANGE_CELL_H__
#define __SDL_SPATIAL_RANGE_CELL_H__

#include "dataserver/spatial/spatial_type.h"
#include "dataserver/system/page_iterator.h"

namespace sdl { namespace db {

class range_cell : noncopyable {
    using make_cell = std::function<spatial_cell()>;
    using data_type = std::vector<make_cell>;
    using data_iterator = data_type::const_iterator;
    using state_type = std::pair<spatial_cell, data_iterator>;
    data_type m_data;
public:
    using iterator = forward_iterator<range_cell const, state_type>;
    range_cell() = default;
    range_cell(range_cell && src) noexcept 
		: m_data(std::move(src.m_data)) {
	}
    void swap(range_cell & src) noexcept {
        m_data.swap(src.m_data);
    }
    range_cell & operator=(range_cell && v) noexcept {
        m_data.swap(v.m_data);
        return *this;
    }
    iterator begin() const;
    iterator end() const;
    template<class T>
    void push_back(T && data) {
        m_data.push_back(std::forward<T>(data));
    }
private:
    friend iterator;
    spatial_cell dereference(state_type const & it) const {
        SDL_ASSERT(it.first);
        return it.first;
    }
    void load_next(state_type & state) const {
        auto & it = state.second;
        const auto last = m_data.end();
        while (it != last) {
            if ((state.first = (*it)())) {
                break;
            }
            ++it;
        }
    }
};

} // db
} // sdl

#endif // __SDL_SPATIAL_RANGE_CELL_H__