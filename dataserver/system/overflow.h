// overflow.h
//
#pragma once
#ifndef __SDL_SYSTEM_OVERFLOW_H__
#define __SDL_SYSTEM_OVERFLOW_H__

#include "page_head.h"

namespace sdl { namespace db {

class database;

class mem_range_page : noncopyable {
protected:
    using data_type = vector_mem_range_t;
    using const_iterator = data_type::const_iterator;
    data_type m_data;
    mem_range_page() = default;
    ~mem_range_page() = default;
public:
    data_type && detach() noexcept {
        static_assert_is_nothrow_move_assignable(data_type);
        return std::move(m_data);
    }
    data_type & data() {
        return m_data;
    }
    const data_type & data() const {
        return m_data;
    }
    const_iterator begin() const {
        return m_data.begin();
    }
    const_iterator end() const {
        return m_data.end();
    }
    size_t length() const {
        return mem_size_n(m_data);
    }
    bool empty() const {
        return m_data.empty();
    }
private:
    std::string text() const;
    std::string ntext() const;
};

// SQL Server stores variable-length column data, which does not exceed 8,000 bytes, on special pages called row-overflow pages
class varchar_overflow_page : public mem_range_page {
public:
    varchar_overflow_page(database const *, overflow_page const *);
};

class varchar_overflow_link : public mem_range_page {
public:
    varchar_overflow_link(database const *, overflow_page const *, overflow_link const *);
};

// For the text, ntext, or image columns, SQL Server stores the data off-row by default. It uses another kind of page called LOB data pages.
// Like ROW_OVERFLOW data, there is a pointer to another piece of information called the LOB root structure, which contains a set of the pointers to other data pages/rows.
class text_pointer_data : public mem_range_page {
public:
    text_pointer_data(database const *, text_pointer const *);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_OVERFLOW_H__
