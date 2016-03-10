// index_tree.cpp
//
#include "common/common.h"
#include "index_tree.h"
#include "database.h"
#include "page_info.h"

namespace sdl { namespace db { 

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
    SDL_ASSERT(sizeof(index_page_row_char) <= head->data.pminlen);
}

bool index_tree::index_page::is_key_NULL() const
{
    return !(slot || head->data.prevPage);
}

//------------------------------------------------------------------------

index_tree::index_tree(database * p, unique_cluster_index && h)
    : db(p), key_length(h->key_length())
{
    cluster = std::move(h);
    SDL_ASSERT(db && cluster && cluster->root);
    SDL_ASSERT(cluster->root->is_index());
    SDL_ASSERT(!(cluster->root->data.prevPage));
    SDL_ASSERT(!(cluster->root->data.nextPage));
    SDL_ASSERT(root()->data.pminlen == key_length + 7);
    SDL_ASSERT(key_length);
}

index_tree::index_page
index_tree::begin_index() const
{
    return index_page(this, page_begin(), 0);
}

index_tree::index_page
index_tree::end_index() const
{
    page_head const * const h = page_end();
    return index_page(this, h, slot_array::size(h));
}

bool index_tree::is_begin_index(index_page const & p) const
{
    return !(p.slot || p.head->data.prevPage);
}

bool index_tree::is_end_index(index_page const & p) const
{
    if (p.slot == p.size()) {
        SDL_ASSERT(!p.head->data.nextPage);
        return true;
    }
    SDL_ASSERT(p.slot < p.size());
    return false;
}

page_head const * index_tree::load_leaf_page(bool const begin) const
{
    page_head const * head = root();
    while (1) {
        const index_page_char page(head);
        const auto row = begin ? page.front() : page.back();
        const char * const p1 = &(row->data.key);
        const char * const p2 = p1 + key_length;
        const auto id = reinterpret_cast<const pageFileID *>(p2);
        if (auto next = db->load_page_head(*id)) {
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

index_tree::row_access::iterator
index_tree::row_access::begin()
{
    return iterator(this, tree->begin_index());
}

index_tree::row_access::iterator
index_tree::row_access::end()
{
    return iterator(this, tree->end_index());
}

void index_tree::row_access::load_next(index_page & p)
{
    tree->load_next_row(p);
}

void index_tree::row_access::load_prev(index_page & p)
{
    tree->load_prev_row(p);
}

bool index_tree::row_access::is_end(index_page const & p) const
{
    return tree->is_end_index(p);
}

bool index_tree::row_access::is_key_NULL(iterator const & it) const
{
    return it.current.is_key_NULL();
}

//----------------------------------------------------------------------

index_tree::page_access::iterator
index_tree::page_access::begin()
{
    return iterator(this, tree->begin_index());
}

index_tree::page_access::iterator
index_tree::page_access::end()
{
    return iterator(this, tree->end_index());
}

void index_tree::page_access::load_next(index_page & p)
{
    tree->load_next_page(p);
}

void index_tree::page_access::load_prev(index_page & p)
{
    tree->load_prev_page(p);
}

bool index_tree::page_access::is_end(index_page const & p) const
{
    return tree->is_end_index(p);
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
    };

} // namespace

std::string index_tree::type_key(key_mem m) const
{
    if (mem_size(m) == key_length) {
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
    }
    SDL_ASSERT(0);
    return{};
}

size_t index_tree::index_page::find_slot(key_mem const m) const
{
    SDL_ASSERT(mem_size(m));
    const index_page_char data(this->head);
    index_page_row_char const * const null = head->data.prevPage ? nullptr : index_page_char(this->head).front();
    size_t i = data.lower_bound([this, &m, null](index_page_row_char const * const x, size_t) {
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

pageFileID index_tree::index_page::find_page(key_mem key) const
{
    return row_page(find_slot(key)); 
}

pageFileID index_tree::find_page(key_mem const m) const
{
    if (mem_size(m) == this->key_length) {
        index_page p(this, root(), 0);
        while (1) {
            auto const id = p.row_page(p.find_slot(m));
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
            SDL_ASSERT(0);
            break;
        }
    }
    SDL_ASSERT(0);
    return{};
}

int index_tree::sub_key_compare(size_t const i, key_mem const & x, key_mem const & y) const
{
    switch ((*cluster)[i].type) {
    case scalartype::t_int:
        if (auto px = index_key_cast<scalartype::t_int>(x)) {
        if (auto py = index_key_cast<scalartype::t_int>(y)) {
            if (cluster->is_descending(i)) {
                std::swap(px, py);
            }
            if ((*px) < (*py)) return -1;
            if ((*py) < (*px)) return 1;
        }}
        break;
    case scalartype::t_bigint:
        if (auto px = index_key_cast<scalartype::t_bigint>(x)) {
        if (auto py = index_key_cast<scalartype::t_bigint>(y)) {
            if (cluster->is_descending(i)) {
                std::swap(px, py);
            }
            if ((*px) < (*py)) return -1;
            if ((*py) < (*px)) return 1;
        }}
        break;
    case scalartype::t_uniqueidentifier:
        if (auto px = index_key_cast<scalartype::t_uniqueidentifier>(x)) {
        if (auto py = index_key_cast<scalartype::t_uniqueidentifier>(y)) {
            if (cluster->is_descending(i)) {
                std::swap(px, py);
            }
            const int val = guid_t::compare(*px, *py);
            if (val < 0) return -1;
            if (val > 0) return 1;
        }}
        break;
    default:
        SDL_ASSERT(0);
        break;
    }
    return 0; // keys are equal
}

bool index_tree::key_less(key_mem x, key_mem y) const
{
    SDL_ASSERT(mem_size(x) == this->key_length);
    SDL_ASSERT(mem_size(y) == this->key_length);
    for (size_t i = 0; i < cluster->size(); ++i) {
        size_t const sz = cluster->sub_key_length(i);
        x.second = x.first + sz;
        y.second = y.first + sz;
        switch (sub_key_compare(i, x, y)) {
        case -1 : return true;
        case 1 : return false;
        default:
            break;
        }
        x.first = x.second;
        y.first = y.second;
    }
    return false; // keys are equal
}

bool index_tree::key_less(vector_mem_range_t const & x, key_mem y) const
{
    SDL_ASSERT(mem_size(x) == this->key_length);
    SDL_ASSERT(mem_size(y) == this->key_length);
    if (x.size() == cluster->size()) {
        for (size_t i = 0; i < cluster->size(); ++i) {
            size_t const sz = cluster->sub_key_length(i);
            y.second = y.first + sz;
            switch (sub_key_compare(i, x[i], y)) {
            case -1 : return true;
            case 1 : return false;
            default:
                break;
            }
            y.first = y.second;
        }
        return false; // keys are equal
    }
    // key values are splitted ?
    throw_error<index_tree_error>("bad key");
    return false;
}


bool index_tree::key_less(key_mem x, vector_mem_range_t const & y) const
{
    SDL_ASSERT(mem_size(x) == this->key_length);
    SDL_ASSERT(mem_size(y) == this->key_length);
    if (y.size() == cluster->size()) {
        for (size_t i = 0; i < cluster->size(); ++i) {
            size_t const sz = cluster->sub_key_length(i);
            x.second = x.first + sz;
            switch (sub_key_compare(i, x, y[i])) {
            case -1 : return true;
            case 1 : return false;
            default:
                break;
            }
            x.first = x.second;
        }
        return false; // keys are equal
    }
    // key values are splitted ?
    throw_error<index_tree_error>("bad key");
    return false;
}

} // db
} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    SDL_TRACE_FILE;
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG