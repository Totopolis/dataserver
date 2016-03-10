// overflow.h
//
#ifndef __SDL_SYSTEM_OVERFLOW_H__
#define __SDL_SYSTEM_OVERFLOW_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

class database;

class mem_range_page : noncopyable {
protected:
    vector_mem_range_t m_data;
    mem_range_page() = default;
    ~mem_range_page() = default;
public:
    const vector_mem_range_t & data() const {
        return m_data;
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
    varchar_overflow_page(database *, overflow_page const *);
};

class varchar_overflow_link : public mem_range_page {
public:
    varchar_overflow_link(database *, overflow_page const *, overflow_link const *);
};

// For the text, ntext, or image columns, SQL Server stores the data off-row by default. It uses another kind of page called LOB data pages.
// Like ROW_OVERFLOW data, there is a pointer to another piece of information called the LOB root structure, which contains a set of the pointers to other data pages/rows.
class text_pointer_data : public mem_range_page {
public:
    text_pointer_data(database *, text_pointer const *);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_OVERFLOW_H__
