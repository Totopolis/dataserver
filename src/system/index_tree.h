// index_tree.h
//
#ifndef __SDL_SYSTEM_INDEX_TREE_H__
#define __SDL_SYSTEM_INDEX_TREE_H__

#pragma once

#include "datapage.h"
#include "page_iterator.h"
#include "database.h"

namespace sdl { namespace db {

template<class T>
class index_tree: noncopyable {
public:
    using key_type = T;
    using index_page_row = index_page_row_t<T>;
    using value_type = index_page_row const *;
private:
    using index_page = datapage_t<index_page_row>;
    database * const db;       
    page_head const * const root;
private:
    class index_access { // level index scan
        page_head const * head;
        size_t slot_index;
        friend index_tree;
    public:
        explicit index_access(page_head const * p, size_t i = 0)
            : head(p), slot_index(i)
        {
            SDL_ASSERT(head);
            SDL_ASSERT(head->data.pminlen == sizeof(index_page_row));
            SDL_ASSERT(slot_array::size(head));
        }
        value_type operator*() const {
            return index_page(head)[slot_index];
        }
        bool operator == (index_access const & x) const {
            return (head == x.head) && (slot_index == x.slot_index);
        }
    };
    static value_type dereference(index_access const & p) { return *p; }
    void load_next(index_access&);
    void load_prev(index_access&);
    bool is_end(index_access const &);
    bool is_begin(index_access const &);
public:
    using iterator = page_iterator<index_tree, index_access>;
    friend iterator;

    explicit index_tree(database * p, page_head const * h)
        : db(p), root(h)
    {
        SDL_ASSERT(db && root);
        SDL_ASSERT(root->data.type == db::pageType::type::index);  
        SDL_ASSERT(!(root->data.prevPage));
        SDL_ASSERT(!(root->data.nextPage));
    }
    iterator begin() {
        return iterator(this, index_access(root));
    }
    iterator end() {
        return iterator(this, index_access(root, slot_array::size(root)));
    }
    template<class fun_type>
    void for_reverse(fun_type fun);
};

template<class T>
bool index_tree<T>::is_end(index_access const & p)
{
    SDL_ASSERT(p.slot_index <= slot_array::size(p.head));
    return p.slot_index == slot_array::size(p.head);
}

template<class T>
bool index_tree<T>::is_begin(index_access const & p)
{
    if (!p.slot_index) {
        return !(p.head->data.prevPage);
    }
    return false;
}

template<class T>
void index_tree<T>::load_next(index_access & p)
{
    SDL_ASSERT(!is_end(p));
    if (++p.slot_index == slot_array::size(p.head)) {
        if (auto h = db->load_next_head(p.head)) {
            p.slot_index = 0;
            p.head = h;
        }
    }
}

template<class T>
void index_tree<T>::load_prev(index_access & p)
{
    SDL_ASSERT(!is_begin(p));
    if (p.slot_index) {
        --p.slot_index;
    }
    else {
        if (auto h = db->load_prev_head(p.head)) {
            p.head = h;
            const size_t size = slot_array::size(h);
            p.slot_index = size ? (size - 1) : 0;
        }
        else {
            SDL_ASSERT(0);
        }
    }
    SDL_ASSERT(!is_end(p));
}

template<class T>
template<class fun_type>
void index_tree<T>::for_reverse(fun_type fun)
{
    iterator last = begin();
    iterator p = end();
    if (p != last) {
        do {
            --p;
            fun(*p);
        } while (p != last);
    }
}

template<scalartype::type v>
using index_tree_t = index_tree<typename index_key<v>::type>;

} // db
} // sdl

#endif // __SDL_SYSTEM_INDEX_TREE_H__
