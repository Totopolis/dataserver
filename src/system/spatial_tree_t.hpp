// spatial_tree_t.hpp
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TREE_T_HPP__
#define __SDL_SYSTEM_SPATIAL_TREE_T_HPP__

namespace sdl { namespace db {

template<typename KEY_TYPE>
spatial_tree_t<KEY_TYPE>::spatial_tree_t(database * const p, 
                           page_head const * const h, 
                           shared_primary_key const & pk0,
                           sysidxstats_row const * const idx)
    : this_db(p), cluster_root(h), idxstat(idx)
{
    A_STATIC_ASSERT_TYPE(impl::scalartype_to_key<scalartype::t_bigint>::type, int64);
    SDL_ASSERT(this_db && cluster_root && idxstat);
    if (pk0 && pk0->is_index() && is_index(cluster_root)) {
        SDL_ASSERT(1 == pk0->size());                           //FIXME: current implementation
        SDL_ASSERT(pk0->first_type() == scalartype::t_bigint);  //FIXME: current implementation
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
void spatial_tree_t<KEY_TYPE>::datapage_access::load_next(state_type & p)
{
    p = detail(tree->this_db)->load_next_head(p);
}

template<typename KEY_TYPE>
void spatial_tree_t<KEY_TYPE>::datapage_access::load_prev(state_type & p)
{
    if (p) {
        SDL_ASSERT(p != tree->min_page());
        SDL_ASSERT(p->data.prevPage);
        p = detail(tree->this_db)->load_prev_head(p);
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
        if (auto next = detail(this_db)->load_page_head(row->data.page)) {
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

template<typename KEY_TYPE>
page_head const * spatial_tree_t<KEY_TYPE>::min_page() const
{
    if (!_min_page) {
        _min_page = load_leaf_page(true); 
    }
    return _min_page;
}

template<typename KEY_TYPE>
page_head const * spatial_tree_t<KEY_TYPE>::max_page() const
{
    if (!_max_page) {
        _max_page = load_leaf_page(false); 
    }
    return _max_page;
}

template<typename KEY_TYPE>
spatial_page_row const * spatial_tree_t<KEY_TYPE>::min_page_row() const
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
spatial_page_row const * spatial_tree_t<KEY_TYPE>::max_page_row() const
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
    size_t i = data.lower_bound([&cell_id, null](spatial_tree_row const * const x, size_t) {
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
        spatial_index data(head);
        auto const & id = data[find_slot(data, cell_id)]->data.page;
        if (auto const next = detail(this_db)->load_page_head(id)) {
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
bool spatial_tree_t<KEY_TYPE>::intersect(spatial_page_row const * p, cell_ref c)
{
    SDL_ASSERT(p);
    return p ? p->data.cell_id.intersect(c) : false;
}

template<typename KEY_TYPE>
bool spatial_tree_t<KEY_TYPE>::is_front_intersect(page_head const * const h, cell_ref cell_id)
{
    SDL_ASSERT(h->is_data());
    spatial_datapage data(h);
    return data.front()->data.cell_id.intersect(cell_id);
}

template<typename KEY_TYPE>
bool spatial_tree_t<KEY_TYPE>::is_back_intersect(page_head const * const h, cell_ref cell_id)
{
    SDL_ASSERT(h->is_data());
    spatial_datapage data(h);
    return data.back()->data.cell_id.intersect(cell_id);
}

template<typename KEY_TYPE>
page_head const * spatial_tree_t<KEY_TYPE>::page_lower_bound(cell_ref cell_id) const
{
    auto const id = find_page(cell_id);
    if (page_head const * h = detail(this_db)->load_page_head(id)) {
        while (is_front_intersect(h, cell_id)) {
            if (auto h2 = detail(this_db)->load_prev_head(h)) {
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
spatial_page_row const * spatial_tree_t<KEY_TYPE>::load_page_row(recordID const & pos) const
{
    if (page_head const * const h = detail(this_db)->load_page_head(pos.id)) {
        SDL_ASSERT(is_data(h) && (pos.slot < slot_array(h).size()));
        return spatial_datapage(h)[pos.slot];
    }
    SDL_ASSERT(!pos);
    return nullptr;
}

template<typename KEY_TYPE>
recordID spatial_tree_t<KEY_TYPE>::find(cell_ref cell_id) const
{
    if (page_head const * const h = page_lower_bound(cell_id)) {
        const spatial_datapage data(h);
        if (data) {
            size_t const slot = data.lower_bound(
                [&cell_id](spatial_page_row const * const row, size_t) {
                auto const & row_cell = row->data.cell_id;
                return (row_cell < cell_id) && !row_cell.intersect(cell_id);
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
void spatial_tree_t<KEY_TYPE>::for_range(spatial_cell const & c1, spatial_cell const & c2, fun_type fun) const
{
    SDL_ASSERT(c1 && c2);
    SDL_ASSERT((c1 == c2) || !c1.intersect(c2));
    if (!(c2 < c1)) {
        recordID it = find(c1);
        if (it) {
            while (spatial_page_row const * const p = load_page_row(it)) {
                auto const & row_cell = p->data.cell_id;
                if ((row_cell < c2) || row_cell.intersect(c2)) {
                    if (fun(p)) {
                        it = detail(this_db)->load_next_record(it);
                        continue;
                    }
                }
                break;
            }
        }
    }
    else {
        SDL_ASSERT(0);
    }
}

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TREE_T_HPP__