// index_tree.cpp
//
#include "dataserver/common/common.h"
#include "dataserver/system/index_tree.h"
#include "dataserver/system/database.h"
#include "dataserver/system/page_info.h"

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
    SDL_ASSERT(sizeof(index_page_row_key) <= head->data.pminlen);
}

//------------------------------------------------------------------------

index_tree::index_tree(database const * p, shared_cluster_index const & h)
    : this_db(p), cluster(h), key_length(h->key_length())
{
    SDL_ASSERT(this_db && cluster && root());
    SDL_ASSERT(root()->is_index());
    SDL_ASSERT(!(root()->data.prevPage));
    SDL_ASSERT(!(root()->data.nextPage));
    SDL_ASSERT(root()->data.pminlen == key_length + 7);
    SDL_ASSERT(key_length);
}

page_head const * 
index_tree::load_leaf_page(bool const begin) const
{
    page_head const * head = root();
    while (1) {
        const index_page_key page(head);
        const auto row = begin ? page.front() : page.back();
        const char * const p1 = &(row->data.key);
        const char * const p2 = p1 + key_length;
        const auto id = reinterpret_cast<const pageFileID *>(p2);
        if (auto next = this_db->load_page_head(*id)) {
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

//----------------------------------------------------------------------

void index_tree::load_prev_row(index_page & p) const
{
    SDL_ASSERT(!is_begin_index(p));
    if (p.slot) {
        --p.slot;
    }
    else {
        if (auto next = this_db->load_prev_head(p.head)) {
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
        if (auto next = this_db->load_next_head(p.head)) {
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
    if (auto next = this_db->load_next_head(p.head)) {
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
        if (auto next = this_db->load_prev_head(p.head)) {
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

    struct type_key_fun : noncopyable
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
        void unexpected(scalartype::type const v) {
            switch (v) {
            case scalartype::t_char:
                result = std::string(data.first, data.second);
                break;
            case scalartype::t_nchar:
                result = to_string::type(make_nchar_checked(data));
                break;
            default:
                result = to_string::dump_mem(data);
                break;
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
        for (size_t i = 0, end = cluster.size(); i < end; ++i) {
            m.second = m.first + cluster.sub_key_length(i);
            std::string s;
            case_index_key(cluster[i].type, type_key_fun(s, m));
            SDL_ASSERT(!s.empty());
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
    const index_page_key data(this->head);
    index_page_row_key const * const null = head->data.prevPage ? nullptr : index_page_key(this->head).front();
    size_t i = data.lower_bound([this, &m, null](index_page_row_key const * const x) {
        return (x == null) || tree->key_less(get_key(x), m);
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

pageFileID index_tree::find_page(key_mem const m) const
{
    if (mem_size(m) == this->key_length) {
        index_page p(this, root(), 0);
        while (1) {
            auto const & id = p.row_page(p.find_slot(m));
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
                SDL_ASSERT(0);
            }
            break;
        }
    }
    SDL_ASSERT(0);
    return{};
}

template<class fun_type>
pageFileID index_tree::find_page_if(fun_type && fun) const
{
    index_page p(this, root(), 0);
    while (1) {
        auto const & id = fun(p);
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

pageFileID index_tree::min_page() const
{
    auto const id = find_page_if([](index_page const & p){
        return p.min_page();
    });
    SDL_ASSERT(id && !this_db->prevPageID(id));
    return id;
}

pageFileID index_tree::max_page() const
{
    auto const id = find_page_if([](index_page const & p){
        return p.max_page();
    });
    SDL_ASSERT(id && !this_db->nextPageID(id));
    return id;
}

int index_tree::sub_key_compare(size_t const i, key_mem const & x, key_mem const & y) const
{
    SDL_ASSERT(mem_size(x) == mem_size(y));
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
    case scalartype::t_char:
        {
            if (cluster->is_descending(i))
                return ::memcmp(y.first, x.first, mem_size(x));
            else
                return ::memcmp(x.first, y.first, mem_size(x));
        }
        break;
    case scalartype::t_nchar:
        {
            SDL_ASSERT(!(mem_size(x) % 2));
            const size_t N = mem_size(x) / 2;
            nchar_t const * px = reinterpret_cast<nchar_t const *>(x.first);
            nchar_t const * py = reinterpret_cast<nchar_t const *>(y.first);
            if (cluster->is_descending(i)) {
                std::swap(px, py);
            }
            return nchar_compare(px, py, N);
        }
        break; 
    default:
        SDL_ASSERT(0); // not implemented
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
