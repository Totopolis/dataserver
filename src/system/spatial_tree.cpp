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
    SDL_ASSERT(head->data.pminlen == sizeof(spatial_root_row));
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
}

page_head const * spatial_tree::load_leaf_page(bool const begin) const
{
    page_head const * head = cluster_root;
    while (1) {
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
                return head;
            }
            SDL_ASSERT(head->data.pminlen == sizeof(spatial_root_row));
        }
        else {
            SDL_ASSERT(0);
            break;
        }
    }
    throw_error<spatial_tree_error>("bad index");
    return nullptr;
}

void spatial_tree::load_next_page(index_page & p) const
{
    SDL_ASSERT(!is_end_index(p));
    SDL_ASSERT(!p.slot);
    if (auto next = this_db->load_next_head(p.head)) {
        p.head = next;
        p.slot = 0;
    }
    else {
        p.slot = p.size();
    }
}

void spatial_tree::load_prev_page(index_page & p) const
{
    SDL_ASSERT(!is_begin_index(p));
    if (!p.slot) {
        if (auto next = this_db->load_prev_head(p.head)) {
            p.head = next;
            p.slot = 0;
        }
        else {
            throw_error<spatial_tree_error>("bad index");
        }
    }
    else {
        SDL_ASSERT(is_end_index(p));
        p.slot = 0;
    }
}

spatial_cell spatial_tree::min_cell() const
{
    if (auto const p = page_begin()) {
        const index_page_key page(p);
        if (page.size() > 1) {
            SDL_ASSERT(index_page(this, p, 0).is_key_NULL(0));
            return page[1]->data.cell_id; // slot 0 has NULL cell_id
        }
    }
    SDL_ASSERT(0);
    return{};
}

spatial_cell spatial_tree::max_cell() const
{
    if (auto const p = page_end()) {
        const index_page_key page(p);
        if (page.size() > 1) {
            return page[page.size()-1]->data.cell_id;
        }
    }
    SDL_ASSERT(0);
    return{};
}

spatial_tree::vector_pk0
spatial_tree::find(spatial_cell const & c1, spatial_cell const & c2) const
{
    SDL_TRACE_FUNCTION;
    SDL_ASSERT(!(c2 < c1));
    SDL_ASSERT(c1 && c2);
    SDL_TRACE(to_string::type(c1));
    SDL_TRACE(to_string::type(c2));
    return{};
}

size_t spatial_tree::index_page::find_slot(key_ref cell_id) const
{
    const index_page_key data(this->head);
    spatial_root_row const * const null = head->data.prevPage ? nullptr : index_page_key(this->head).front();
    size_t i = data.lower_bound([this, &cell_id, null](spatial_root_row const * const x, size_t) {
        if (x == null)
            return true;
        return spatial_tree::key_less(get_key(x), cell_id);
    });
    SDL_ASSERT(i <= data.size());
    if (i < data.size()) {
        if (i && spatial_tree::key_less(cell_id, row_key(i))) {
            --i;
        }
        return i;
    }
    SDL_ASSERT(i);
    return i - 1; // last slot
}

pageFileID spatial_tree::find_page(key_ref cell_id) const
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
                return id;
            }
        }
        break;
    }
    SDL_ASSERT(0);
    return{};
}

spatial_tree::unique_datarow_access
spatial_tree::get_datarow(key_ref cell_id) const
{
    if (auto const head = this_db->load_page_head(find_page(cell_id))) {
        return sdl::make_unique<datarow_access>(head);
    }
    return{};
}

} // db
} // sdl

/*#pragma pack(push, 1)
    struct key_type {
        spatial_cell    cell_id;
        pk0_type        pk0;
    };
#pragma pack(pop)
    static_assert(sizeof(key_type) == 13, "");
    static size_t const key_length = sizeof(key_type);
    using index_page_row_key = index_page_row_t<key_type>;
    using index_page_key = datapage_t<index_page_row_key>;
    using key_ref = key_type const &;
    using row_mem = index_page_row_key::data_type const &;*/

/*struct clustered {
    struct T0 {
        using type = spatial_cell;
    };
    struct T1 {
        using type = pk0_type;
    };
    struct key_type {
        T0::type    _0; // cell_id
        T1::type    _1; // pk0
        using this_clustered = clustered;
    };
};*/