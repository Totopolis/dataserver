// maketable_scan.hpp
//
#pragma once
#ifndef __SDL_SYSTEM_MAKETABLE_SCAN_HPP__
#define __SDL_SYSTEM_MAKETABLE_SCAN_HPP__

namespace sdl { namespace db { namespace make {

template<class this_table, class record>
record make_query<this_table, record>::find_with_index(key_type const & key) const {
    static_assert(index_size, "");
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
                        return get_record(head);
                    }
                }
            }
        }
    }
    return {};
}

template<class this_table, class record>
std::pair<page_slot, bool>
make_query<this_table, record>::lower_bound(T0_type const & value) const
{
    static_assert(index_size, "");
    auto const db = m_table.get_db();
    if (auto const id = make::index_tree<key_type>(db, m_cluster).first_page(value)) {
        if (page_head const * const h = db->load_page_head(id)) {
            SDL_ASSERT(h->is_data());
            const datapage data(h);
            if (!data.empty()) {
                const size_t slot = data.lower_bound([this, &value](row_head const * const row, size_t) {
                    return this->key_less<T0_col>(row, value);
                });
                if (slot < data.size()) {
                    const bool is_equal = !this->key_less<T0_col>(value, data[slot]);
                    SDL_ASSERT(is_equal == meta::is_equal<T0_col>::equal(value, col_value<T0_col>(data[slot])));
                    return { page_slot(h, slot), is_equal };
                }
                auto next = db->load_next_head(h);
                while (next) {
                    if (!datapage(next).empty()) {
                        return { page_slot(next), false };
                    }
                    SDL_WARNING(0); // to be tested
                    next = db->load_next_head(next);
                }
            }
        }
    }
    return {};
}

template<class this_table, class record>
template<class fun_type>
void make_query<this_table, record>::scan_next(page_slot const & pos, fun_type fun) const
{
    SDL_ASSERT(pos.page);
    static_assert(index_size, "");
    auto const db = m_table.get_db();
    if (pos.page) {
        size_t slot = pos.slot;
        page_head const * page = pos.page;
        SDL_ASSERT(slot < datapage(page).size());
        while (page) {
            const datapage data(page);
            while (slot < data.size()) {
                if (!fun(get_record(data[slot]))) {
                    return;
                }
                ++slot;
            }
            page = db->load_next_head(page);
            slot = 0;
        }
    }
}

template<class this_table, class record>
template<class fun_type>
void make_query<this_table, record>::scan_prev(page_slot const & pos, fun_type fun) const
{
    SDL_ASSERT(pos.page);
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
                        return;
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
}

#if 0
template<class this_table, class record>
//template<class fun_type, class is_equal_type> break_or_continue
template<class fun_type> break_or_continue
make_query<this_table, record>::_scan_with_index(T0_type const & value, fun_type fun) const //, is_equal_type is_equal) const
{
    if (0) {
        auto found = lower_bound(value);
        if (found.second) {

        }
    }
    static_assert(index_size, "");
    auto const db = m_table.get_db();
    if (auto const id = make::index_tree<key_type>(db, m_cluster).first_page(value)) {
        if (page_head const * h = db->load_page_head(id)) {
            SDL_ASSERT(h->is_data());
            const datapage data(h);
            if (!data.empty()) {
                size_t slot = data.lower_bound([this, &value](row_head const * const row, size_t) {
                    return this->key_less<T0_col>(row, value);
                });
                //FIXME: rename scan_with_index to lower_bound and return recordID (fileId:pageId:slot)
                //FIXME: add scan_if from recordID with direction forward/backward...
                if (slot < data.size()) {
                    {
                        if (this->key_less<T0_col>(value, data[slot])) {
                            return bc::continue_;
                        }
                        if (fun(this->get_record(data[slot])) == bc::break_) {
                            return bc::break_;
                        }
                        ++slot;
                    }
                    auto const last = data.end();
                    for (auto it = data.begin_slot(slot); it != last; ++it) {
                        const record current = this->get_record(*it);
                        if (meta::is_equal<T0_col>::equal(current.val(identity<T0_col>{}), value)) {
                            if (fun(current) == bc::break_) {
                                return bc::break_;
                            }
                        }
                        else {
                            return bc::continue_;                        
                        }
                    }
                    while ((h = db->load_next_head(h)) != nullptr) {
                        SDL_ASSERT(h->is_data());
                        const datapage next_data(h);
                        auto const last = next_data.end();
                        for (auto it = next_data.begin(); it != last; ++it) {
                            const record current = this->get_record(*it);
                            if (meta::is_equal<T0_col>::equal(current.val(identity<T0_col>{}), value)) {
                                if (fun(current) == bc::break_) {
                                    return bc::break_;
                                }
                            }
                            else {
                                return bc::continue_;
                            }
                        }
                    }
                    return bc::continue_;
                }
            }
        }
    }
    return bc::continue_;
}
#endif

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_MAKETABLE_SCAN_HPP__
