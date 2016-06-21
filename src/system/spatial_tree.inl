// spatial_tree.inl
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TREE_INL__
#define __SDL_SYSTEM_SPATIAL_TREE_INL__

namespace sdl { namespace db { 

inline spatial_tree::index_page
spatial_tree::begin_index() const
{
    return index_page(this, page_begin(), 0);
}

inline spatial_tree::index_page
spatial_tree::end_index() const
{
    page_head const * const h = page_end();
    return index_page(this, h, slot_array::size(h));
}

inline bool spatial_tree::is_begin_index(index_page const & p) const
{
    return !(p.slot || p.head->data.prevPage);
}

inline bool spatial_tree::is_end_index(index_page const & p) const
{
    if (p.slot == p.size()) {
        SDL_ASSERT(!p.head->data.nextPage);
        return true;
    }
    SDL_ASSERT(p.slot < p.size());
    return false;
}

//----------------------------------------------------------------

inline bool spatial_tree::index_page::is_key_NULL(size_t const slot) const {
    return !(slot || head->data.prevPage);
}

inline spatial_tree::row_mem
spatial_tree::index_page::operator[](size_t const i) const {
    return index_page_key(this->head)[i]->data;
}

//----------------------------------------------------------------

inline spatial_tree::page_access::iterator
spatial_tree::page_access::begin()
{
    return iterator(this, tree->begin_index());
}

inline spatial_tree::page_access::iterator
spatial_tree::page_access::end()
{
    return iterator(this, tree->end_index());
}

inline void spatial_tree::page_access::load_next(index_page & p)
{
    tree->load_next_page(p);
}

inline void spatial_tree::page_access::load_prev(index_page & p)
{
    tree->load_prev_page(p);
}

inline bool spatial_tree::page_access::is_end(index_page const & p) const
{
    return tree->is_end_index(p);
}

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TREE_INL__
