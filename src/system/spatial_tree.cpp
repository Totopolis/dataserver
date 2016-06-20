// spatial_tree.cpp
//
#include "common/common.h"
#include "spatial_tree.h"
#include "database.h"
#include "page_info.h"
#include "index_tree_t.h"

namespace sdl { namespace db {

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

class spatial_tree::data_t : noncopyable {
#pragma pack(push, 1)
    struct key_type {
        spatial_cell    cell_id;
        pk0_type        pk0;
    };
#pragma pack(pop)
    static_assert(sizeof(key_type) == 13, "");
    static size_t const key_length = sizeof(key_type);
    using index_page_row_key = index_page_row_t<key_type>;
    using index_page_key = datapage_t<index_page_row_key>;
public:
    data_t(database *, page_head const *);
    
    database * this_db() const { return this->db; }
    page_head const * root() const { return cluster_root; }
    
    spatial_cell min_cell() const;
    spatial_cell max_cell() const;

    vector_pk0 find(cell_range const &) const;
private:
    page_head const * load_leaf_page(bool const begin) const;
    page_head const * page_begin() const {
        return load_leaf_page(true);
    }
    page_head const * page_end() const {
        return load_leaf_page(false);
    }
private:
    database * const db;
    page_head const * const cluster_root;
};

spatial_tree::data_t::data_t(database * p, page_head const * h)
    : db(p), cluster_root(h)
{
    SDL_ASSERT(db && cluster_root);
    SDL_ASSERT(root()->is_index());
    SDL_ASSERT(!(root()->data.prevPage));
    SDL_ASSERT(!(root()->data.nextPage));
    SDL_ASSERT(root()->data.pminlen == key_length + 7);
}

page_head const * spatial_tree::data_t::load_leaf_page(bool const begin) const
{
    page_head const * head = root();
    while (1) {
        const index_page_key page(head);
        const auto row = begin ? page.front() : page.back();
        if (auto next = this_db()->load_page_head(row->data.page)) {
            if (next->is_index()) {
                head = next;
            }
            else {
                SDL_ASSERT(next->is_data());
                SDL_ASSERT(!begin || !head->data.prevPage);
                SDL_ASSERT(begin || !head->data.nextPage);
                return head;
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

spatial_cell spatial_tree::data_t::min_cell() const
{
    if (auto const p = page_begin()) {
        const index_page_key page(p);
        if (page.size() > 1) {
            return page[1]->data.key.cell_id;
        }
    }
    SDL_ASSERT(0);
    return{};
}

spatial_cell spatial_tree::data_t::max_cell() const
{
    if (auto const p = page_end()) {
        const index_page_key page(p);
        if (page.size() > 1) {
            return page[page.size()-1]->data.key.cell_id;
        }
    }
    SDL_ASSERT(0);
    return{};
}

spatial_tree::vector_pk0
spatial_tree::data_t::find(cell_range const & rg) const
{
    SDL_ASSERT(!(rg.second < rg.first));
    SDL_ASSERT(rg.first.data.depth && rg.second.data.depth);
    return{};
}

//----------------------------------------------------------------------------

spatial_tree::spatial_tree(database * const db, page_head const * const h, shared_primary_key const & pk0)
{
    SDL_ASSERT(db && h);
    if (pk0 && pk0->is_index()) {
        SDL_ASSERT(1 == pk0->size());                           //FIXME: current implementation
        SDL_ASSERT(pk0->first_type() == scalartype::t_bigint);  //FIXME: current implementation
        A_STATIC_ASSERT_TYPE(impl::scalartype_to_key<scalartype::t_bigint>::type, int64);
        m_data = sdl::make_unique<data_t>(db, h);
    }
    throw_error_if<spatial_tree_error>(!m_data.get(), "spatial_tree");
}

spatial_tree::~spatial_tree()
{
}

spatial_cell spatial_tree::min_cell() const
{
    return m_data->min_cell();
}

spatial_cell spatial_tree::max_cell() const
{
    return m_data->max_cell();
}

spatial_tree::vector_pk0
spatial_tree::find(cell_range const & rg) const
{
    return m_data->find(rg);
}

} // db
} // sdl

