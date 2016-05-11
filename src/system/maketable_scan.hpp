// maketable_scan.hpp
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_SCAN_HPP__
#define __SDL_SYSTEM_MAKETABLE_SCAN_HPP__

namespace sdl { namespace db { namespace make {

template<class this_table, class record>
record make_query<this_table, record>::find_with_index(key_type const & key) {
    SDL_ASSERT(m_cluster);
    auto const db = m_table.get_db();
    if (auto const id = make::index_tree<key_type>(db, m_cluster).find_page(key)) {
        if (page_head const * const h = db->load_page_head(id)) {
            SDL_ASSERT(h->is_data());
            const datapage data(h);
            if (!data.empty()) {
                size_t const slot = data.lower_bound(
                    [this, &key](row_head const * const row, size_t) {
                    return (this->read_key(row) < key);
                });
                if (slot < data.size()) {
                    row_head const * const head = data[slot];
                    if (!(key < read_key(head))) {
                        return record(&m_table, head);
                    }
                }
            }
        }
    }
    return {};
}

template<class this_table, class record>
template<class value_type, class fun_type, template<class col> class is_equal_type> break_or_continue
make_query<this_table, record>::scan_with_index(value_type const & value, fun_type fun, is_equal_type<first_key> is_equal)
{
    using col = typename table_clustered::T0::col;
    A_STATIC_ASSERT_TYPE(value_type, decltype(key_type()._0));
    SDL_ASSERT(m_cluster);
    static_assert(index_size > 1, "");
    auto const db = m_table.get_db();
    if (auto const id = make::index_tree<key_type>(db, m_cluster).first_page(value)) {
        if (page_head const * h = db->load_page_head(id)) {
            SDL_ASSERT(h->is_data());
            const datapage data(h);
            if (!data.empty()) {
                size_t slot = data.lower_bound([this, &value](row_head const * const row, size_t) {
                    return this->get_record(row).val(identity<col>{}) < value;
                });
                if (slot < data.size()) {
                    {
                        const record current = this->get_record(data[slot]);
                        if (value < current.val(identity<col>{})) {
                            return continue_;
                        }
                        if (fun(current) == break_) {
                            return break_;
                        }
                        ++slot;
                    }
                    auto const last = data.end();
                    for (auto it = data.begin_slot(slot); it != last; ++it) {
                        const record current = this->get_record(*it);
                        if (is_equal(current.val(identity<col>{}), value)) {
                            if (fun(current) == break_) {
                                return break_;
                            }
                        }
                        else {
                            return continue_;                        
                        }
                    }
                    while ((h = db->load_next_head(h)) != nullptr) {
                        SDL_ASSERT(h->is_data());
                        const datapage next_data(h);
                        auto const last = next_data.end();
                        for (auto it = next_data.begin(); it != last; ++it) {
                            const record current = this->get_record(*it);
                            if (is_equal(current.val(identity<col>{}), value)) {
                                if (fun(current) == break_) {
                                    return break_;
                                }
                            }
                            else {
                                return continue_;
                            }
                        }
                    }
                    return continue_;
                }
            }
        }
    }
    return continue_;
}

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_SCAN_HPP__
