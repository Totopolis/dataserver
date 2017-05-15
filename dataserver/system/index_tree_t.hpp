// index_tree_t.hpp
//
#pragma once
#ifndef __SDL_SYSTEM_INDEX_TREE_T_HPP__
#define __SDL_SYSTEM_INDEX_TREE_T_HPP__

namespace sdl { namespace db { namespace make {

template<typename KEY_TYPE>
index_tree<KEY_TYPE>::index_page::index_page(index_tree const * t, page_head const * h, size_t const i)
    : tree(t), head(h), slot(i)
{
    SDL_ASSERT(tree && head);
    SDL_ASSERT(head->is_index());
    SDL_ASSERT(head->data.pminlen == index_tree::key_length + index_row_head_size);
    SDL_ASSERT(slot <= slot_array::size(head));
    SDL_ASSERT(slot_array::size(head));
    SDL_ASSERT(sizeof(index_page_row_key) <= head->data.pminlen);
}

template<typename KEY_TYPE>
index_tree<KEY_TYPE>::index_tree(database const * const p, page_head const * const h)
    : this_db(p), cluster_root(h)
{
    SDL_ASSERT(this_db && cluster_root);
    SDL_ASSERT(root()->is_index());
    SDL_ASSERT(!(root()->data.prevPage));
    SDL_ASSERT(!(root()->data.nextPage));
    SDL_ASSERT(root()->data.pminlen == key_length + 7);
    A_STATIC_ASSERT_TYPE(first_key, decltype(KEY_TYPE()._0));
    A_STATIC_ASSERT_TYPE(first_key, typename KEY_TYPE::this_clustered::T0::type);
}

template<typename KEY_TYPE>
page_head const * 
index_tree<KEY_TYPE>::load_leaf_page(bool const begin) const
{
    page_head const * head = root();
    while (1) {
        const index_page_key page(head);
        const auto row = begin ? page.front() : page.back();
        if (auto next = fwd::load_page_head(this_db, row->data.page)) {
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
    throw_error<index_tree_error>("bad index");
    return nullptr;
}

template<typename KEY_TYPE>
void index_tree<KEY_TYPE>::load_prev_row(index_page & p) const
{
    SDL_ASSERT(!is_begin_index(p));
    if (p.slot) {
        --p.slot;
    }
    else {
        if (auto next = fwd::load_prev_head(this_db, p.head)) {
            SDL_ASSERT(next->is_index());
            p.head = next;
            p.slot = slot_array::size(next)-1;
        }
        else {
            SDL_ASSERT(0);
        }
    }
}

template<typename KEY_TYPE>
void index_tree<KEY_TYPE>::load_next_row(index_page & p) const
{
    SDL_ASSERT(!is_end_index(p));
    if (++p.slot == p.size()) {
        if (auto next = fwd::load_next_head(this_db, p.head)) {
            SDL_ASSERT(next->is_index());
            p.head = next;
            p.slot = 0;
        }
    }
}

template<typename KEY_TYPE>
void index_tree<KEY_TYPE>::load_next_page(index_page & p) const
{
    SDL_ASSERT(!is_end_index(p));
    SDL_ASSERT(!p.slot);
    if (auto next = fwd::load_next_head(this_db, p.head)) {
        p.head = next;
        p.slot = 0;
    }
    else {
        p.slot = p.size();
    }
}

template<typename KEY_TYPE>
void index_tree<KEY_TYPE>::load_prev_page(index_page & p) const
{
    SDL_ASSERT(!is_begin_index(p));
    if (!p.slot) {
        if (auto next = fwd::load_prev_head(this_db, p.head)) {
            p.head = next;
            p.slot = 0;
        }
        else {
            throw_error<index_tree_error>("bad index");
        }
    }
    else {
        SDL_ASSERT(is_end_index(p));
        p.slot = 0;
    }
}

template<typename KEY_TYPE>
size_t index_tree<KEY_TYPE>::index_page::find_slot(key_ref m) const
{
    const index_page_key data(this->head);
    index_page_row_key const * const null = head->data.prevPage ? nullptr : index_page_key(this->head).front();
    size_t i = data.lower_bound([this, &m, null](index_page_row_key const * const x) {
        if (x == null)
            return true;
        return index_tree::key_less(get_key(x), m);
    });
    SDL_ASSERT(i <= data.size());
    if (i < data.size()) {
        if (i && index_tree::key_less(m, row_key(i))) {
            --i;
        }
        return i;
    }
    SDL_ASSERT(i);
    return i - 1; // last slot
}

template<typename KEY_TYPE>
size_t index_tree<KEY_TYPE>::index_page::first_slot(first_key const & m) const
{
    const index_page_key data(this->head);
    index_page_row_key const * const null = head->data.prevPage ? nullptr : index_page_key(this->head).front();
    size_t i = data.lower_bound([this, &m, null](index_page_row_key const * const x) {
        if (x == null)
            return true;
        return index_tree::less_first(get_key(x)._0, m);
    });
    SDL_ASSERT(i <= data.size());
    if (i < data.size()) {
        if (i && index_tree::less_first(m, row_key(i)._0)) {
            --i;
        }
        return i;
    }
    SDL_ASSERT(i);
    return i - 1; // last slot
}

template<typename KEY_TYPE>
pageFileID index_tree<KEY_TYPE>::find_page(key_ref m) const
{
    index_page p(this, root(), 0);
    while (1) {
        auto const & id = p.row_page(p.find_slot(m));
        if (auto const head = fwd::load_page_head(this_db, id)) {
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

template<typename KEY_TYPE> 
template<typename make_query_type>
pageFileID index_tree<KEY_TYPE>::first_page_clustered(first_key const & m, make_query_type const & query, bool_constant<false>) const
{
    static_assert(KEY_TYPE::this_clustered::index_size == 1, "");
    index_page p(this, root(), 0);
    while (1) {
        const pageFileID id = p.row_page(p.first_slot(m));
        if (auto const head = fwd::load_page_head(this_db, id)) {
            if (head->is_index()) {
                p.head = head;
                p.slot = 0;
                continue;
            }
            if (head->is_data()) {
                SDL_ASSERT(id == head->data.pageId);
                return id;
            }
        }
        break;
    }
    SDL_ASSERT(0);
    return{};
}

template<typename KEY_TYPE> 
template<typename make_query_type>
pageFileID index_tree<KEY_TYPE>::first_page_clustered(first_key const & m, make_query_type const & query, bool_constant<true>) const
{
    static_assert(KEY_TYPE::this_clustered::index_size > 1, "");
    index_page p(this, root(), 0);
    while (1) {
        const pageFileID id = p.row_page(p.first_slot(m));
        if (auto const head = fwd::load_page_head(this_db, id)) {
            if (head->is_index()) {
                p.head = head;
                p.slot = 0;
                continue;
            }
            if (head->is_data()) {
                SDL_ASSERT(id == head->data.pageId);
                page_head const * found = head;
                for (;;) {
                    if (page_head const * prev = fwd::load_prev_head(this_db, found)) {
                        SDL_ASSERT(prev->is_data());
                        const datapage data(prev);
                        if (!data.empty()) {
                            if (query.equal_first_key(data.back(), m)) {
                                found = prev;
                                continue;
                            }
                        }
                    }
                    break;
                }
                return found->data.pageId;
            }
        }
        break;
    }
    SDL_ASSERT(0);
    return{};
}

template<typename KEY_TYPE> 
template<typename make_query_type> inline
pageFileID index_tree<KEY_TYPE>::first_page(first_key const & m, make_query_type const & query) const
{
    enum { multiple_keys = KEY_TYPE::this_clustered::index_size > 1 };
    return first_page_clustered(m, query, bool_constant<multiple_keys>());
}

template<typename KEY_TYPE>
template<class fun_type>
pageFileID index_tree<KEY_TYPE>::find_page_if(fun_type fun) const
{
    index_page p(this, root(), 0);
    while (1) {
        auto const & id = fun(p);
        if (auto const head = fwd::load_page_head(this_db, id)) {
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

template<typename KEY_TYPE> inline
pageFileID index_tree<KEY_TYPE>::min_page() const
{
    auto const id = find_page_if([](index_page const & p){
        return p.min_page();
    });
    SDL_ASSERT(id && !fwd::prevPageID(this_db, id));
    return id;
}

template<typename KEY_TYPE> inline
pageFileID index_tree<KEY_TYPE>::max_page() const
{
    auto const id = find_page_if([](index_page const & p){
        return p.max_page();
    });
    SDL_ASSERT(id && !fwd::nextPageID(this_db, id));
    return id;
}

} // make
} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_TREE_T_HPP__