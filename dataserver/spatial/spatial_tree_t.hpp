// spatial_tree_t.hpp
//
#pragma once
#ifndef __SDL_SPATIAL_SPATIAL_TREE_T_HPP__
#define __SDL_SPATIAL_SPATIAL_TREE_T_HPP__

namespace sdl { namespace db {

template<typename KEY_TYPE>
spatial_tree_t<KEY_TYPE>::spatial_tree_t(database const * const p, 
                           page_head const * const h, 
                           shared_primary_key const & pk0,
                           sysidxstats_row const * const idx)
    : this_db(p), cluster_root(h), idxstat(idx)
{
    A_STATIC_ASSERT_TYPE(scalartype_t<scalartype::t_bigint>, int64);
    SDL_ASSERT(this_db && cluster_root && idxstat);
    SDL_ASSERT(1 == pk0->size()); //FIXME: current implementation
    if (pk0 && pk0->is_index() && is_index(cluster_root) && (1 == pk0->size())) {
        SDL_ASSERT(pk0->first_type() == key_to_scalartype<pk0_type>::value);
        m_min_page = load_leaf_page(true);
        m_max_page = load_leaf_page(false); 
    }
    else {
        throw_error<spatial_tree_error>("bad index");
    }
    SDL_ASSERT(is_str_empty(spatial_datapage::name()));
    SDL_ASSERT(find_page(min_cell()));
    SDL_ASSERT(find_page(max_cell()));
}

template<typename KEY_TYPE>
bool spatial_tree_t<KEY_TYPE>::is_index(page_head const * const h)
{
    if (h && h->is_index() && slot_array::size(h)) {
        SDL_ASSERT(h->data.pminlen == sizeof(spatial_tree_row));
        return true;
    }
    return false;
}

template<typename KEY_TYPE>
bool spatial_tree_t<KEY_TYPE>::is_data(page_head const * const h)
{
    if (h && h->is_data() && slot_array::size(h)) {
        SDL_ASSERT(h->data.pminlen == sizeof(spatial_page_row));
        return true;
    }
    return false;
}

template<typename KEY_TYPE> inline
void spatial_tree_t<KEY_TYPE>::datapage_access::load_next(state_type & p) const
{
    p = fwd::load_next_head(tree->this_db, p);
}

template<typename KEY_TYPE> inline
void spatial_tree_t<KEY_TYPE>::datapage_access::load_prev(state_type & p) const
{
    if (p) {
        SDL_ASSERT(p != tree->min_page());
        SDL_ASSERT(p->data.prevPage);
        p = fwd::load_prev_head(tree->this_db, p);
    }
    else {
        p = tree->max_page();
    }
}

template<typename KEY_TYPE>
page_head const * spatial_tree_t<KEY_TYPE>::load_leaf_page(bool const begin) const
{
    page_head const * head = cluster_root;
    while (1) {
        SDL_ASSERT(is_index(head));
        const spatial_index page(head);
        const auto row = begin ? page.front() : page.back();
        if (auto next = fwd::load_page_head(this_db, row->data.page)) {
            if (next->is_index()) {
                head = next;
            }
            else {
                SDL_ASSERT(is_data(next));
                SDL_ASSERT(!begin || !head->data.prevPage);
                SDL_ASSERT(begin || !head->data.nextPage);
                SDL_ASSERT(!begin || !next->data.prevPage);
                SDL_ASSERT(begin || !next->data.nextPage);
                return next;
            }
        }
        else {
            SDL_ASSERT(0);
            break;
        }
    }
    throw_error<spatial_tree_error>("bad index");
    return nullptr;
}

template<typename KEY_TYPE> inline
page_head const * spatial_tree_t<KEY_TYPE>::min_page() const
{
    SDL_ASSERT(m_min_page);
    return m_min_page;
}

template<typename KEY_TYPE> inline
page_head const * spatial_tree_t<KEY_TYPE>::max_page() const
{
    SDL_ASSERT(m_max_page);
    return m_max_page;
}

template<typename KEY_TYPE>
typename spatial_tree_t<KEY_TYPE>::spatial_page_row const *
spatial_tree_t<KEY_TYPE>::min_page_row() const
{
    if (auto const p = min_page()) {
        const spatial_datapage page(p);
        if (!page.empty()) {
            return page.front();
        }
    }
    SDL_ASSERT(0);
    return{};
}

template<typename KEY_TYPE>
typename spatial_tree_t<KEY_TYPE>::spatial_page_row const *
spatial_tree_t<KEY_TYPE>::max_page_row() const
{
    if (auto const p = max_page()) {
        const spatial_datapage page(p);
        if (!page.empty()) {
            return page.back();
        }
    }
    SDL_ASSERT(0);
    return{};
}

template<typename KEY_TYPE>
spatial_cell spatial_tree_t<KEY_TYPE>::min_cell() const
{
    if (auto const p = min_page_row()) {
        return p->data.cell_id;
    }
    SDL_ASSERT(0);
    return{};
}

template<typename KEY_TYPE>
spatial_cell spatial_tree_t<KEY_TYPE>::max_cell() const
{
    if (auto const p = max_page_row()) {
        return p->data.cell_id;
    }
    SDL_ASSERT(0);
    return{};
}

template<typename KEY_TYPE>
size_t spatial_tree_t<KEY_TYPE>::find_slot(spatial_index const & data, cell_ref cell_id)
{
    spatial_tree_row const * const null = data.prevPage() ? nullptr : data.front();
    size_t i = data.lower_bound([&cell_id, null](spatial_tree_row const * const x) {
        if (x == null)
            return true;
        return x->data.key.cell_id < cell_id;
    });
    SDL_ASSERT(i <= data.size());
    if (i < data.size()) {
        if (i && (cell_id < data[i]->data.key.cell_id)) {
            --i;
        }
        return i;
    }
    SDL_ASSERT(i);
    return i - 1; // last slot
}

template<typename KEY_TYPE>
pageFileID spatial_tree_t<KEY_TYPE>::find_page(cell_ref cell_id) const
{
    SDL_ASSERT(cell_id);
    page_head const * head = cluster_root;
    while (1) {
        SDL_ASSERT(is_index(head));
        const spatial_index data(head);
        auto const & id = data[find_slot(data, cell_id)]->data.page;
        if (auto const next = fwd::load_page_head(this_db, id)) {
            if (next->is_index()) {
                head = next;
                continue;
            }
            if (next->is_data()) {
                SDL_ASSERT(id);
                return id;
            }
            SDL_ASSERT(0);
        }
        break;
    }
    SDL_ASSERT(0);
    return{};
}

template<typename KEY_TYPE>
template<class fun_type> break_or_continue
spatial_tree_t<KEY_TYPE>::for_each_index_page(spatial_index const & p, fun_type && fun) const
{
    if (make_break_or_continue(fun(p.head)) == bc::break_) {
        return bc::break_;
    }
    for (size_t i = 0, end = p.size(); i != end; ++i) {
        spatial_tree_row const * const row = p[i];
        if (auto const head = fwd::load_page_head(this_db, row->data.page)) {
            if (head->is_index()) {
                const spatial_index child(head);
                if (is_break(for_each_index_page(child, fun))) {
                    return bc::break_;
                }
            }
            else {
                if (make_break_or_continue(fun(head)) == bc::break_) {
                    return bc::break_;
                }
            }
        }
    }
    return bc::continue_;
}

template<typename KEY_TYPE>
template<class fun_type> break_or_continue
spatial_tree_t<KEY_TYPE>::for_each_index_page(fun_type && fun) const
{
    page_head const * const head = cluster_root;
    SDL_ASSERT(is_index(head));
    const spatial_index data(head);
    return for_each_index_page(data, fun);
}

template<typename KEY_TYPE> inline
bool spatial_tree_t<KEY_TYPE>::intersect(spatial_page_row const * p, cell_ref c)
{
    SDL_ASSERT(p);
    return p ? p->data.cell_id.intersect(c) : false;
}

template<typename KEY_TYPE> inline
bool spatial_tree_t<KEY_TYPE>::is_front_intersect(page_head const * const h, cell_ref cell_id)
{
    SDL_ASSERT(h->is_data());
    return spatial_datapage(h).front()->data.cell_id.intersect(cell_id);
}

template<typename KEY_TYPE> inline
bool spatial_tree_t<KEY_TYPE>::is_back_intersect(page_head const * const h, cell_ref cell_id)
{
    SDL_ASSERT(h->is_data());
    return spatial_datapage(h).back()->data.cell_id.intersect(cell_id);
}

template<typename KEY_TYPE>
page_head const * spatial_tree_t<KEY_TYPE>::page_lower_bound(cell_ref cell_id) const
{
    auto const id = find_page(cell_id);
    if (page_head const * h = fwd::load_page_head(this_db, id)) {
        while (is_front_intersect(h, cell_id)) {
            if (auto h2 = fwd::load_prev_head(this_db, h)) {
                if (is_back_intersect(h2, cell_id)) {
                    h = h2;
                }
                else {
                    break;
                }
            }
            else {
                break;
            }
        }
        SDL_ASSERT(is_data(h));
        return h;
    }
    SDL_ASSERT(0);
    return nullptr;
}

template<typename KEY_TYPE>
typename spatial_tree_t<KEY_TYPE>::spatial_page_row const *
spatial_tree_t<KEY_TYPE>::load_page_row(recordID const & pos) const
{
    if (page_head const * const h = fwd::load_page_head(this_db, pos.id)) {
        SDL_ASSERT(is_data(h) && (pos.slot < slot_array(h).size()));
        return spatial_datapage(h)[pos.slot];
    }
    SDL_ASSERT(!pos);
    return nullptr;
}

template<typename KEY_TYPE>
recordID spatial_tree_t<KEY_TYPE>::find_cell(cell_ref cell_id) const
{
    SDL_ASSERT(cell_id);
    if (page_head const * const h = page_lower_bound(cell_id)) {
        const spatial_datapage data(h);
        if (data) {
            size_t const slot = data.lower_bound(
                [&cell_id](spatial_page_row const * const row) {
                return (row->data.cell_id < cell_id) && !row->data.cell_id.intersect(cell_id);
            });
            if (slot == data.size()) {
                return {};
            }
            return recordID::init(h->data.pageId, slot);
        }
        SDL_ASSERT(0);
    }
    return {};
}

template<typename KEY_TYPE>
template<class fun_type>
break_or_continue spatial_tree_t<KEY_TYPE>::for_cell(cell_ref c1, fun_type && fun) const // try optimize
{
    using depth_t = spatial_cell::id_type;
    A_STATIC_ASSERT_TYPE(uint8, depth_t);
    SDL_ASSERT(c1);
    spatial_cell c2{};
    depth_t const max_depth = c1.data.depth;
    recordID it; // uninitialized
    spatial_page_row const * last = nullptr;
    for (depth_t i = 1; i <= max_depth; ++i) {
        c2.data.depth = i;
        c2.data.id.cell[i - 1] = c1.data.id.cell[i - 1];
        if ((it = find_cell(c2))) {
            spatial_page_row const * p = load_page_row(it);
            if (p != last) {
                if (!last || (last->data.cell_id < p->data.cell_id)) {
                    while (p->data.cell_id.intersect(c1)) {
                        if (make_break_or_continue(fun(p)) == bc::break_) {
                            return bc::break_;
                        }
                        last = p;
                        if ((it = fwd::load_next_record(this_db, it))) {
                            p = load_page_row(it);
                            SDL_ASSERT(p);
                        }
                        else {
                            break;
                        }
                    }
                }
            }
        }
    }
    return bc::continue_;
}

template<typename KEY_TYPE>
template<class fun_type> break_or_continue 
spatial_tree_t<KEY_TYPE>::for_range(spatial_point const & p, Meters const radius, fun_type && fun) const
{
    SDL_TRACE_DEBUG_2("for_range(", p.latitude, ",",  p.longitude, ",", radius.value(), ")");
    sparse_pk0_type set_pk0; // check processed records
    auto function = [this, &fun, &set_pk0](spatial_cell cell) {
        return this->for_cell(cell, [&fun, &set_pk0](spatial_page_row const * const row) {
            if (set_pk0.insert(row->data.pk0)) {
                return make_break_or_continue(fun(row));
            }
            return bc::continue_;
        });
    };
    return transform::cell_range(function_cell_t<decltype(function)>(std::move(function)), p, radius);    
}

template<typename KEY_TYPE>
template<class fun_type> break_or_continue
spatial_tree_t<KEY_TYPE>::for_rect(spatial_rect const & rc, fun_type && fun) const
{
    SDL_TRACE_DEBUG_2("for_rect(", rc.min_lat, ",",  rc.min_lon, ",", rc.max_lat, ",", rc.max_lon, ")");
    sparse_pk0_type set_pk0; // check processed records
    auto function = [this, &fun, &set_pk0](spatial_cell cell){
        return this->for_cell(cell, [&fun, &set_pk0](spatial_page_row const * const row) {
            if (set_pk0.insert(row->data.pk0)) {
                return make_break_or_continue(fun(row));
            }
            return bc::continue_;
        });
    };
    return transform::cell_rect(function_cell_t<decltype(function)>(std::move(function)), rc);
}

template<typename KEY_TYPE>
template<class fun_type> break_or_continue 
spatial_tree_t<KEY_TYPE>::for_range_pk0(spatial_point const & p, Meters const radius, fun_type && fun) const
{
    SDL_TRACE_DEBUG_2("for_range(", p.latitude, ",",  p.longitude, ",", radius.value(), ")");
    sparse_pk0_type set_pk0; // check processed records
    auto function = [this, &set_pk0](spatial_cell cell) {
        return this->for_cell(cell, [&set_pk0](spatial_page_row const * const row) {
            set_pk0.insert(row->data.pk0);
            return bc::continue_;
        });
    };
    transform::cell_range(function_cell_t<decltype(function)>(std::move(function)), p, radius);
    return set_pk0.for_each(std::forward<fun_type>(fun));
}

template<typename KEY_TYPE>
template<class fun_type> break_or_continue
spatial_tree_t<KEY_TYPE>::for_rect_pk0(spatial_rect const & rc, fun_type && fun) const
{
    SDL_TRACE_DEBUG_2("for_rect(", rc.min_lat, ",",  rc.min_lon, ",", rc.max_lat, ",", rc.max_lon, ")");
    sparse_pk0_type set_pk0; // check processed records
    auto function = [this, &set_pk0](spatial_cell cell){
        return this->for_cell(cell, [&set_pk0](spatial_page_row const * const row) {
            set_pk0.insert(row->data.pk0);
            return bc::continue_;
        });
    };
    transform::cell_rect(function_cell_t<decltype(function)>(std::move(function)), rc);
    return set_pk0.for_each(std::forward<fun_type>(fun));
}

template<typename KEY_TYPE>
typename spatial_tree_t<KEY_TYPE>::sparse_pk0_type
spatial_tree_t<KEY_TYPE>::for_range_pk0(spatial_point const & p, Meters radius) const
{
    SDL_TRACE_DEBUG_2("for_range(", p.latitude, ",", p.longitude, ",", radius.value(), ")");
    sparse_pk0_type set_pk0; // check processed records
    auto function = [this, &set_pk0](spatial_cell cell) {
        return this->for_cell(cell, [&set_pk0](spatial_page_row const * const row) {
            set_pk0.insert(row->data.pk0);
            return bc::continue_;
        });
    };
    transform::cell_range(function_cell_t<decltype(function)>(std::move(function)), p, radius);
    return set_pk0;
}

template<typename KEY_TYPE>
typename spatial_tree_t<KEY_TYPE>::sparse_pk0_type
spatial_tree_t<KEY_TYPE>::for_rect_pk0(spatial_rect const & rc) const
{
    SDL_TRACE_DEBUG_2("for_rect(", rc.min_lat, ",", rc.min_lon, ",", rc.max_lat, ",", rc.max_lon, ")");
    sparse_pk0_type set_pk0; // check processed records
    auto function = [this, &set_pk0](spatial_cell cell) {
        return this->for_cell(cell, [&set_pk0](spatial_page_row const * const row) {
            set_pk0.insert(row->data.pk0);
            return bc::continue_;
        });
    };
    transform::cell_rect(function_cell_t<decltype(function)>(std::move(function)), rc);
    return set_pk0;
}

template<typename KEY_TYPE>
template<class fun_type>
break_or_continue spatial_tree_t<KEY_TYPE>::full_globe(fun_type && fun) const
{
    page_head const * h = min_page();
    while (h) {
        const spatial_datapage data(h);
        for (auto const p : data) {
            A_STATIC_CHECK_TYPE(spatial_page_row const * const, p);
            if (make_break_or_continue(fun(p)) == bc::break_) {
                return bc::break_;
            }
        }
#if SDL_DEBUG
        const auto next = fwd::load_next_head(this_db, h);
        SDL_ASSERT((h == m_max_page) == (next == nullptr));
        h = next;
#else
        h = fwd::load_next_head(this_db, h);
#endif
    }
    return bc::continue_;
}

} // db
} // sdl

#endif // __SDL_SPATIAL_SPATIAL_TREE_T_HPP__