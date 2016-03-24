// index_tree_t.cpp
//
#include "common/common.h"
#include "index_tree_t.h"
#include "database.h"
#include "page_info.h"

namespace sdl { namespace db { namespace todo {

index_tree::index_page::index_page(index_tree const * t, page_head const * h, size_t const i)
    : tree(t)
    , head(h)
    , slot(i)
{
    SDL_ASSERT(tree && head);
    SDL_ASSERT(head->is_index());
    SDL_ASSERT(head->data.pminlen == tree->key_length + index_row_head_size);
    SDL_ASSERT(slot <= slot_array::size(head));
    SDL_ASSERT(slot_array::size(head));
    SDL_ASSERT(sizeof(index_page_row_key) <= head->data.pminlen);
}

//------------------------------------------------------------------------

index_tree::index_tree(database * p, shared_cluster_index const & h)
    : db(p), cluster(h)
{
    SDL_ASSERT(db && cluster && root());
    SDL_ASSERT(root()->is_index());
    SDL_ASSERT(!(root()->data.prevPage));
    SDL_ASSERT(!(root()->data.nextPage));
    SDL_ASSERT(root()->data.pminlen == key_length + 7);
    SDL_ASSERT(h->key_length() == index_tree::key_length);
}

page_head const * index_tree::load_leaf_page(bool const begin) const
{
    page_head const * head = root();
    while (1) {
        const index_page_key page(head);
        const auto row = begin ? page.front() : page.back();
        if (auto next = db->load_page_head(row->data.page)) {
            if (next->is_index()) {
                head = next;
            }
            else {
                SDL_ASSERT(next->is_data());
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

//----------------------------------------------------------------------

void index_tree::load_prev_row(index_page & p) const
{
    SDL_ASSERT(!is_begin_index(p));
    if (p.slot) {
        --p.slot;
    }
    else {
        if (auto next = db->load_prev_head(p.head)) {
            SDL_ASSERT(next->is_index());
            p.head = next;
            p.slot = slot_array::size(next)-1;
        }
        else {
            SDL_ASSERT(0);
        }
    }
}

void index_tree::load_next_row(index_page & p) const
{
    SDL_ASSERT(!is_end_index(p));
    if (++p.slot == p.size()) {
        if (auto next = db->load_next_head(p.head)) {
            SDL_ASSERT(next->is_index());
            p.head = next;
            p.slot = 0;
        }
    }
}

void index_tree::load_next_page(index_page & p) const
{
    SDL_ASSERT(!is_end_index(p));
    SDL_ASSERT(!p.slot);
    if (auto next = db->load_next_head(p.head)) {
        p.head = next;
        p.slot = 0;
    }
    else {
        p.slot = p.size();
    }
}

void index_tree::load_prev_page(index_page & p) const
{
    SDL_ASSERT(!is_begin_index(p));
    if (!p.slot) {
        if (auto next = db->load_prev_head(p.head)) {
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

//----------------------------------------------------------------------

namespace {

    struct type_key_fun
    {
        std::string & result;
        mem_range_t const data;

        type_key_fun(std::string & s, mem_range_t const & m): result(s), data(m) {}

        template<class T> // T = index_key_t
        void operator()(T) {
            if (auto pv = index_key_cast<T::value>(data)) {
                result = to_string::type(*pv);
            }
        }
        void unexpected(scalartype::type) {
            result = to_string::dump_mem(data);
        }
    };

} // namespace

std::string index_tree::type_key(key_ref const m) const
{
    /*if (mem_size(m) == key_length) {
        std::string result;
        cluster_index const & cluster = index();
        bool const multiple = (cluster.size() > 1);
        if (multiple) result += "(";
        for (size_t i = 0; i < cluster.size(); ++i) {
            m.second = m.first + cluster.sub_key_length(i);
            std::string s;
            case_index_key(cluster[i].type, type_key_fun(s, m));
            if (i) result += ",";
            result += s;
            m.first = m.second;
        }
        if (multiple) result += ")";
        return result;
    }*/
    SDL_ASSERT(0);
    return{};
}

size_t index_tree::index_page::find_slot(key_ref const m) const
{
    const index_page_key data(this->head);
    index_page_row_key const * const null = head->data.prevPage ? nullptr : index_page_key(this->head).front();
    size_t i = data.lower_bound([this, &m, null](index_page_row_key const * const x, size_t) {
        if (x == null)
            return true;
        return tree->key_less(get_key(x), m);
    });
    SDL_ASSERT(i <= data.size());
    if (i < data.size()) {
        if (i && tree->key_less(m, row_key(i))) {
            --i;
        }
        return i;
    }
    SDL_ASSERT(i);
    return i - 1; // last slot
}

pageFileID index_tree::find_page(key_ref const m) const
{
    index_page p(this, root(), 0);
    while (1) {
        auto const & id = p.row_page(p.find_slot(m));
        if (auto const head = db->load_page_head(id)) {
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

template<class fun_type>
pageFileID index_tree::find_page_if(fun_type fun) const
{
    index_page p(this, root(), 0);
    while (1) {
        auto const & id = fun(p);
        if (auto const head = db->load_page_head(id)) {
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

pageFileID index_tree::min_page() const
{
    auto const id = find_page_if([](index_page const & p){
        return p.min_page();
    });
    SDL_ASSERT(id && !db->prevPageID(id));
    return id;
}

pageFileID index_tree::max_page() const
{
    auto const id = find_page_if([](index_page const & p){
        return p.max_page();
    });
    SDL_ASSERT(id && !db->nextPageID(id));
    return id;
}

} // todo
} // db
} // sdl