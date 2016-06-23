// spatial_tree.cpp
//
#include "common/common.h"
#include "spatial_tree.h"
#include "database.h"
#include "page_info.h"
#include "index_tree_t.h"

namespace sdl { namespace db {

spatial_tree::index_page::index_page(spatial_tree const * t, page_head const * h, size_t const i)
    : tree(t), head(h), slot(i)
{
    SDL_ASSERT(tree && head);
    SDL_ASSERT(head->is_index());
    SDL_ASSERT(head->data.pminlen == sizeof(spatial_tree_row));
    SDL_ASSERT(slot <= slot_array::size(head));
    SDL_ASSERT(slot_array::size(head));
}

spatial_tree::spatial_tree(database * const p, page_head const * const h, shared_primary_key const & pk0)
    : this_db(p), cluster_root(h), grid(spatial_grid::HIGH)
{
    SDL_ASSERT(this_db && cluster_root);
    if (pk0 && pk0->is_index()) {
        SDL_ASSERT(1 == pk0->size());                           //FIXME: current implementation
        SDL_ASSERT(pk0->first_type() == scalartype::t_bigint);  //FIXME: current implementation
        A_STATIC_ASSERT_TYPE(impl::scalartype_to_key<scalartype::t_bigint>::type, int64);
    }
    else {
        throw_error<spatial_tree_error>("spatial_tree");
    }
    SDL_ASSERT(is_str_empty(spatial_datapage::name()));
}

spatial_tree::datapage_access::iterator
spatial_tree::datapage_access::begin()
{
    page_head const * p = tree->min_page();
    return iterator(this, std::move(p));
}

spatial_tree::datapage_access::iterator
spatial_tree::datapage_access::end()
{
    return iterator(this, nullptr);
}

spatial_tree::unique_datapage
spatial_tree::datapage_access::dereference(state_type const p)
{
    SDL_ASSERT(p);
    return make_unique<spatial_datapage>(p);
}

void spatial_tree::datapage_access::load_next(state_type & p)
{
    p = tree->this_db->load_next_head(p);
}

void spatial_tree::datapage_access::load_prev(state_type & p)
{
    if (p) {
        SDL_ASSERT(p != tree->min_page());
        SDL_ASSERT(p->data.prevPage);
        p = tree->this_db->load_prev_head(p);
    }
    else {
        p = tree->max_page();
    }
}

page_head const * spatial_tree::load_leaf_page(bool const begin) const
{
    page_head const * head = cluster_root;
    while (1) {
        SDL_ASSERT(head->data.pminlen == sizeof(spatial_tree_row));
        const index_page_key page(head);
        const auto row = begin ? page.front() : page.back();
        if (auto next = this_db->load_page_head(row->data.page)) {
            if (next->is_index()) {
                head = next;
            }
            else {
                SDL_ASSERT(next->is_data());
                SDL_ASSERT(!begin || !head->data.prevPage);
                SDL_ASSERT(begin || !head->data.nextPage);
                SDL_ASSERT(!begin || !next->data.prevPage);
                SDL_ASSERT(begin || !next->data.nextPage);
                SDL_ASSERT(next->data.pminlen == sizeof(spatial_page_row));
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

page_head const * spatial_tree::min_page() const
{
    if (!_min_page) {
        _min_page = load_leaf_page(true); 
    }
    return _min_page;
}

page_head const * spatial_tree::max_page() const
{
    if (!_max_page) {
        _max_page = load_leaf_page(false); 
    }
    return _max_page;
}

spatial_page_row const * spatial_tree::min_page_row() const
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
spatial_page_row const * spatial_tree::max_page_row() const
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

spatial_cell spatial_tree::min_cell() const
{
    if (auto const p = min_page_row()) {
        return p->data.cell_id;
    }
    SDL_ASSERT(0);
    return{};
}

spatial_cell spatial_tree::max_cell() const
{
    if (auto const p = max_page_row()) {
        return p->data.cell_id;
    }
    SDL_ASSERT(0);
    return{};
}

size_t spatial_tree::index_page::find_slot(cell_ref cell_id) const
{
    const index_page_key data(this->head);
    spatial_tree_row const * const null = head->data.prevPage ? nullptr : index_page_key(this->head).front();
    size_t i = data.lower_bound([this, &cell_id, null](spatial_tree_row const * const x, size_t) {
        if (x == null)
            return true;
        return get_cell(x) < cell_id;
    });
    SDL_ASSERT(i <= data.size());
    if (i < data.size()) {
        if (i && (cell_id < row_cell(i))) {
            --i;
        }
        return i;
    }
    SDL_ASSERT(i);
    return i - 1; // last slot
}

pageFileID spatial_tree::find_page(cell_ref cell_id) const
{
    SDL_ASSERT(cell_id);
    index_page p(this, cluster_root, 0);
    while (1) {
        auto const & id = p.row_page(p.find_slot(cell_id));
        if (auto const head = this_db->load_page_head(id)) {
            if (head->is_index()) {
                p.head = head;
                p.slot = 0;
                continue;
            }
            if (head->is_data()) {
                SDL_ASSERT(id);
                return id;
            }
        }
        break;
    }
    SDL_ASSERT(0);
    return{};
}

bool spatial_tree::is_front(page_head const * h, cell_ref cell_id)
{
    SDL_ASSERT(h->is_data());
    spatial_datapage data(h);
    if (!data.empty()) {
        return data.front()->data.cell_id == cell_id;
    }
    SDL_ASSERT(0);
    return false;
}

bool spatial_tree::is_back(page_head const * h, cell_ref cell_id)
{
    SDL_ASSERT(h->is_data());
    spatial_datapage data(h);
    if (!data.empty()) {
        return data.back()->data.cell_id == cell_id;
    }
    SDL_ASSERT(0);
    return false;
}

page_head const * spatial_tree::lower_bound(cell_ref cell_id) const
{
    if (page_head const * h = this_db->load_page_head(find_page(cell_id))) {
        while (is_front(h, cell_id)) {
            if (page_head const * h2 = this_db->load_prev_head(h)) {
                if (is_back(h2, cell_id)) {
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
        return h;
    }
    SDL_ASSERT(0);
    return nullptr;
}

recordID spatial_tree::find(cell_ref cell_id) const
{
    if (page_head const * const h = lower_bound(cell_id)) {
        const spatial_datapage data(h);
        size_t const slot = data.lower_bound(
            [&cell_id](spatial_page_row const * const row, size_t) {
            return row->data.cell_id < cell_id;
        });
        SDL_ASSERT(slot <= data.size());
        if (slot == data.size()) {
            return {};
        }
        return recordID::init(h->data.pageId, slot);
    }
    return {};
}

void spatial_tree::for_range(spatial_cell const & c1, spatial_cell const & c2) const
{
    SDL_ASSERT(!(c2 < c1));
    SDL_ASSERT(c1 && c2);
    SDL_TRACE(to_string::type(c1));
    SDL_TRACE(to_string::type(c2));
}

} // db
} // sdl
