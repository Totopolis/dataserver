// maketable_scan.hpp
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_SCAN_HPP__
#define __SDL_SYSTEM_MAKETABLE_SCAN_HPP__

namespace sdl { namespace db { namespace make {

template<class this_table, class record>
record make_query<this_table, record>::find_with_index(key_type const & key, pageType_t<pageType::type::index>) const
{
    static_assert(index_size, "");
    static_assert(is_cluster_root_index(), "");
    auto const db = m_table.get_db();
    if (auto const id = make::index_tree<key_type>(db, m_cluster_index->root()).find_page(key)) {
        if (page_head const * const h = db->load_page_head(id)) {
            SDL_ASSERT(h->is_data());
            const datapage data(h);
            if (!data.empty()) {
                size_t const slot = data.lower_bound(
                    [this, &key](row_head const * const row) {
                    SDL_ASSERT(row->use_record()); //FIXME: check possibility
                    return (this->read_key(row) < key);
                });
                if (slot < data.size()) {
                    row_head const * const head = data[slot];
                    if (!(key < read_key(head))) {
                        return get_record(head);
                    }
                }
            }
        }
    }
    return {};
}

template<class this_table, class record>
record make_query<this_table, record>::find_with_index(key_type const & key, pageType_t<pageType::type::data>) const
{
    static_assert(index_size, "");
    static_assert(is_cluster_root_data(), "");
    if (record found = make_query::find([&key](record const & p){ 
        return !make_query::less_key(p, key); 
    })) {
        if (make_query::equal_key(found, key)) {
            return found;
        }
    }
    return {};
}

template<class this_table, class record> inline
record make_query<this_table, record>::find_with_index(key_type const & key) const {
    return find_with_index(key, pageType_t<table_clustered::root_page_type>());
}

template<class this_table, class record>
std::pair<page_slot, bool>
make_query<this_table, record>::lower_bound(page_head const * page, T0_type const & value) const
{
    SDL_ASSERT(page);
    while (page) {
        SDL_ASSERT(page->is_data());
        const datapage data(page);
        if (!data.empty()) {
            const size_t slot = data.lower_bound([this, &value](row_head const * const row) {
                SDL_ASSERT(row->use_record());
                return this->key_less<T0_col>(row, value);
            });
            if (slot < data.size()) {
                const bool is_equal = !this->key_less<T0_col>(value, data[slot]);
                SDL_ASSERT(is_equal == meta::is_equal<T0_col>::equal(value, col_value<T0_col>(data[slot])));
                return { page_slot(page, slot), is_equal };
            }
        }
        page = m_table.get_db()->load_next_head(page);
    }
    return {};
}

template<class this_table, class record>
std::pair<page_slot, bool>
make_query<this_table, record>::lower_bound(T0_type const & value, pageType_t<pageType::type::index>) const
{
    static_assert(T0_col::order != sortorder::NONE, "");
    static_assert(index_size, "");
    static_assert(is_cluster_root_index(), "");
    SDL_ASSERT(m_cluster_index->is_root_index());
    auto const db = m_table.get_db();
    if (auto const id = make::index_tree<key_type>(db, m_cluster_index->root()).first_page(value, *this)) {
        return lower_bound(db->load_page_head(id), value);
    }
    return {};
}

template<class this_table, class record> inline
std::pair<page_slot, bool>
make_query<this_table, record>::lower_bound(T0_type const & value, pageType_t<pageType::type::data>) const
{
    static_assert(T0_col::order != sortorder::NONE, "");
    static_assert(index_size, "");
    static_assert(is_cluster_root_data(), "");
    SDL_ASSERT(m_cluster_index->is_root_data());
    return lower_bound(m_cluster_index->root(), value);
}

template<class this_table, class record> inline
std::pair<page_slot, bool>
make_query<this_table, record>::lower_bound(T0_type const & value) const {
    return lower_bound(value, pageType_t<table_clustered::root_page_type>());
}

template<class this_table, class record>
template<class fun_type> page_slot
make_query<this_table, record>::scan_next(page_slot const & pos, fun_type && fun) const
{
    static_assert(index_size, "");
    if (pos.page) {
        size_t slot = pos.slot;
        page_head const * page = pos.page;
        SDL_ASSERT(slot < datapage(page).size());
        while (page) {
            const datapage data(page);
            while (slot < data.size()) {
                if (!fun(get_record(data[slot]))) {
                    return { page, slot };
                }
                ++slot;
            }
            page = m_table.get_db()->load_next_head(page);
            slot = 0;
        }
    }
    else {
        SDL_ASSERT(0);
    }
    return {};
}

template<class this_table, class record>
template<class fun_type> page_slot
make_query<this_table, record>::scan_prev(page_slot const & pos, fun_type && fun) const
{
    static_assert(index_size, "");
    auto const db = m_table.get_db();
    if (pos.page) {
        size_t slot = pos.slot;
        page_head const * page = pos.page;
        SDL_ASSERT(slot < datapage(page).size());
        while (page) {
            const datapage data(page);
            if (!data.empty()) {
                if (slot == datapage::none_slot) {
                    slot = data.size() - 1;
                }
                for (;;) {
                    if (!fun(get_record(data[slot]))) {
                        return { page, slot };
                    }
                    if (!slot) {
                        slot = datapage::none_slot;
                        break;
                    }
                    --slot;
                };
            }
            page = db->load_prev_head(page);
        }
    }
    else {
        SDL_ASSERT(0);
    }
    return {};
}

template<class this_table, class record>
bool make_query<this_table, record>::push_unique(record_range & result, record const & p)
{
    if (!result.empty()) {
        key_type temp_key; // uninitialized
        const key_type unique_key = make_query::read_key(p);
        make_query::read_key(temp_key, result.back());
        if (!(temp_key < unique_key)) {
            const auto pos = std::lower_bound(result.begin(), result.end(), unique_key,
                [](record const & x, const key_type & y)
            {
                key_type key; // uninitialized
                make_query::read_key(key, x);
                return key < y;
            });
            if (pos != result.end()) {
                make_query::read_key(temp_key, *pos);
                if (temp_key == unique_key) {
                    return false;
                }
                result.insert(pos, p);
                return true;
            }
        }
    }
    result.push_back(p); 
    return true;
}

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_SCAN_HPP__
