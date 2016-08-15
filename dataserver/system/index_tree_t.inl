// index_tree_t.inl
//
#pragma once
#ifndef __SDL_SYSTEM_INDEX_TREE_T_INL__
#define __SDL_SYSTEM_INDEX_TREE_T_INL__

namespace sdl { namespace db { namespace make {

template<typename KEY_TYPE> inline 
bool index_tree<KEY_TYPE>::index_page::is_key_NULL() const
{
    return !(slot || head->data.prevPage);
}

template<typename KEY_TYPE> inline
typename index_tree<KEY_TYPE>::index_page
index_tree<KEY_TYPE>::begin_index() const
{
    return index_page(this, page_begin(), 0);
}

template<typename KEY_TYPE> inline 
typename index_tree<KEY_TYPE>::index_page
index_tree<KEY_TYPE>::end_index() const
{
    page_head const * const h = page_end();
    return index_page(this, h, slot_array::size(h));
}

template<typename KEY_TYPE> inline
bool index_tree<KEY_TYPE>::is_begin_index(index_page const & p) const
{
    return !(p.slot || p.head->data.prevPage);
}

template<typename KEY_TYPE> inline
bool index_tree<KEY_TYPE>::is_end_index(index_page const & p) const
{
    if (p.slot == p.size()) {
        SDL_ASSERT(!p.head->data.nextPage);
        return true;
    }
    SDL_ASSERT(p.slot < p.size());
    return false;
}

template<typename KEY_TYPE> inline
typename index_tree<KEY_TYPE>::key_ref
index_tree<KEY_TYPE>::index_page::get_key(index_page_row_key const * const row) const {
    return row->data.key;
}

template<typename KEY_TYPE> inline
typename index_tree<KEY_TYPE>::key_ref 
index_tree<KEY_TYPE>::index_page::row_key(size_t const i) const {
    return get_key(index_page_key(this->head)[i]);
}

template<typename KEY_TYPE> inline
pageFileID const & 
index_tree<KEY_TYPE>::index_page::row_page(size_t const i) const {
    return index_page_key(this->head)[i]->data.page;
}

template<typename KEY_TYPE> inline
typename index_tree<KEY_TYPE>::row_mem
index_tree<KEY_TYPE>::index_page::operator[](size_t const i) const {
    return index_page_key(this->head)[i]->data;
}

template<typename KEY_TYPE> inline
pageFileID const & 
index_tree<KEY_TYPE>::index_page::find_page(key_ref key) const {
    return this->row_page(find_slot(key)); 
}

template<typename KEY_TYPE> inline
pageFileID const & 
index_tree<KEY_TYPE>::index_page::min_page() const {
    return this->row_page(0);
}

template<typename KEY_TYPE> inline
pageFileID const & 
index_tree<KEY_TYPE>::index_page::max_page() const {
    return this->row_page(this->size() - 1);
}

//----------------------------------------------------------------------

template<typename KEY_TYPE> inline
typename index_tree<KEY_TYPE>::row_access::iterator
index_tree<KEY_TYPE>::row_access::begin()
{
    return iterator(this, tree->begin_index());
}

template<typename KEY_TYPE> inline
typename index_tree<KEY_TYPE>::row_access::iterator
index_tree<KEY_TYPE>::row_access::end()
{
    return iterator(this, tree->end_index());
}

template<typename KEY_TYPE> inline
void index_tree<KEY_TYPE>::row_access::load_next(index_page & p)
{
    tree->load_next_row(p);
}

template<typename KEY_TYPE> inline
void index_tree<KEY_TYPE>::row_access::load_prev(index_page & p)
{
    tree->load_prev_row(p);
}

template<typename KEY_TYPE> inline
bool index_tree<KEY_TYPE>::row_access::is_key_NULL(iterator const & it) const
{
    return it.current.is_key_NULL();
}

//----------------------------------------------------------------------

template<typename KEY_TYPE> inline
typename index_tree<KEY_TYPE>::page_access::iterator
index_tree<KEY_TYPE>::page_access::begin()
{
    return iterator(this, tree->begin_index());
}

template<typename KEY_TYPE> inline
typename index_tree<KEY_TYPE>::page_access::iterator
index_tree<KEY_TYPE>::page_access::end()
{
    return iterator(this, tree->end_index());
}

template<typename KEY_TYPE> inline
void index_tree<KEY_TYPE>::page_access::load_next(index_page & p)
{
    tree->load_next_page(p);
}

template<typename KEY_TYPE> inline
void index_tree<KEY_TYPE>::page_access::load_prev(index_page & p)
{
    tree->load_prev_page(p);
}

template<typename KEY_TYPE> inline
bool index_tree<KEY_TYPE>::page_access::is_end(index_page const & p) const
{
    return tree->is_end_index(p);
}

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_TREE_T_INL__
