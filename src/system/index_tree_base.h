// index_tree_base.h
//
#pragma once
#ifndef __SDL_SYSTEM_INDEX_TREE_BASE_H__
#define __SDL_SYSTEM_INDEX_TREE_BASE_H__

#include "datapage.h"

namespace sdl { namespace db {

class database;

template<typename KEY_TYPE>
class index_tree_base {
public:
    using key_type = KEY_TYPE;
    using key_ref = key_type const &;
protected:
    using index_page_row_key = index_page_row_t<key_type>;
    using index_page_key = datapage_t<index_page_row_key>;
public:
    using row_mem = typename index_page_row_key::data_type const &;
    static size_t const key_length = sizeof(key_type);
protected:
    ~index_tree_base() = default;
};

template<> class index_tree_base<void> {
protected:
    using index_page_row_key = index_page_row_t<char>;
    using index_page_key = datapage_t<index_page_row_key>;
public:
    using key_mem = mem_range_t;
    using row_mem = std::pair<mem_range_t, pageFileID>;
protected:
    ~index_tree_base() = default;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_TREE_BASE_H__
