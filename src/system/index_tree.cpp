// index_tree.cpp
//
#include "common/common.h"
#include "index_tree.h"
#include "database.h"
#include "page_info.h"

namespace sdl { namespace db { 

index_tree::index_page::index_page(index_tree const * t, size_t const i)
    : tree(t)
    , head(t->root())
    , slot(i)
{
    SDL_ASSERT(tree && head);
    SDL_ASSERT(head->is_index());
    SDL_ASSERT(head->data.pminlen == tree->key_length + index_row_head_size);
    SDL_ASSERT(slot <= slot_array::size(head));
    SDL_ASSERT(slot_array::size(head));
    SDL_ASSERT(sizeof(index_page_row_char) <= head->data.pminlen);
}

//--------------------------------------------------------------------

index_tree::index_tree(database * p, unique_cluster_index && h)
    : db(p), key_length(h->key_length())
{
    cluster = std::move(h);
    SDL_ASSERT(db && cluster && cluster->root);
    SDL_ASSERT(cluster->root->is_index());
    SDL_ASSERT(!(cluster->root->data.prevPage));
    SDL_ASSERT(!(cluster->root->data.nextPage));
    SDL_ASSERT(root()->data.pminlen == key_length + 7);
    SDL_ASSERT(key_length > 0);
}

index_tree::index_page
index_tree::begin_row() const
{
    return index_page(this);
}

index_tree::index_page
index_tree::end_row() const
{
    return index_page(this, slot_array::size(root()));
}

bool index_tree::is_begin_row(index_page const & p) const
{
    if ((p.head == root()) && !p.slot) {
        SDL_ASSERT(p.stack.empty());
        SDL_ASSERT(slot_array::size(p.head));
        return true;
    }
    return false;
}

bool index_tree::is_end_row(index_page const & p) const
{
    if ((p.head == root()) && (p.slot == slot_array::size(p.head))) {
        SDL_ASSERT(p.stack.empty());
        SDL_ASSERT(p.slot);
        return true;
    }
    return false;
}

void index_tree::index_page::push_stack(page_head const * next)
{
    SDL_ASSERT(next->is_index());
    SDL_ASSERT(this->slot < slot_array::size(this->head));
    this->stack.emplace_back(this->head, this->slot);
    this->head = next;
    this->slot = 0;
    SDL_ASSERT(slot_array::size(this->head));
}

void index_tree::load_next_row(index_page & p) const
{
    SDL_ASSERT(!is_end_row(p));
    SDL_ASSERT(p.head->is_index());
    page_head const * const next = db->load_page_head(p.row_page(p.slot));
    if (next) {
        if (next->is_index()) { // intermediate level
            p.push_stack(next);
        }
        else { // leaf level
            SDL_ASSERT(next->is_data());
            SDL_ASSERT(slot_array::size(next));
            SDL_ASSERT(p.slot < slot_array::size(p.head));
            if (++p.slot == slot_array::size(p.head)) {
                while (!p.stack.empty()) {                
                    page_slot const d = p.stack.back();
                    p.stack.pop_back();               
                    p.head = d.first;
                    p.slot = d.second + 1;
                    if (p.slot < slot_array::size(p.head)) {
                        return;
                    }
                    SDL_ASSERT(p.slot == slot_array::size(p.head));
                }
                SDL_ASSERT(is_end_row(p));
            }
        }
    }
    else {
        throw_error<index_tree_error>("bad index");
    }
}

void index_tree::load_next_page(index_page & p) const
{
    while (1) {
        load_next_row(p);
        if (is_end_row(p) || (0 == p.slot))
            break; 
    }
}

bool index_tree::index_page::key_NULL(size_t const i) const
{
    SDL_ASSERT(i < this->size());
    if (0 == i) {
        for (auto const & s : this->stack) {
            SDL_ASSERT(s.first);
            if (s.second) {
                return false;
            }
        }
        return true;
    }
    return false;
}

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
            result += std::move(s);
            m.first = m.second;
        }
        if (multiple) result += ")";
        return result;
    }
    SDL_ASSERT(0);
    return{};
}

index_tree::key_mem
index_tree::index_page::get_key(index_page_row_char const * const row) const
{
    SDL_ASSERT(row);
    const char * const p1 = &(row->data.key);
    const char * const p2 = p1 + tree->key_length;
    SDL_ASSERT(p1 < p2);
    return { p1, p2 };
}

index_tree::key_mem
index_tree::index_page::row_key(size_t const i) const
{
    SDL_ASSERT(i < size());
    return get_key(index_page_char(this->head)[i]);
}

index_tree::row_mem_type
index_tree::index_page::row_data(size_t const i) const
{
    SDL_ASSERT(i < size());
    key_mem const m = row_key(i);
    auto & page = * reinterpret_cast<const pageFileID *>(m.second);
    return { m, page };
}

pageFileID index_tree::index_page::row_page(size_t const i) const
{
    SDL_ASSERT(i < size());
    key_mem const m = row_key(i);
    return * reinterpret_cast<const pageFileID *>(m.second);
}

size_t index_tree::index_page::find_slot(key_mem const m) const
{
    const index_page_char data(this->head);
    index_page_row_char const * const null = key_NULL(0) ? index_page_char(this->head)[0] : nullptr;
    const size_t i = data.lower_bound([this, &m, null](index_page_row_char const * x) {
        if (x == null)
            return true;
        return tree->key_less(get_key(x), m);
    });
    SDL_ASSERT(i <= data.size());
    if (i < data.size()) {
        if (i == 1) {
            if (tree->key_less(m, row_key(1))) {
                return 0;
            }
        }
        return i;
    }
    SDL_ASSERT(i);
    return i - 1; // last slot
}

pageFileID index_tree::index_page::find_page(key_mem m) const
{
    return row_page(find_slot(m)); 
}

pageFileID index_tree::find_page(key_mem const m) const
{
    if (mem_size(m) == this->key_length) {
        index_page p = begin_row();
        while (1) {
            p.slot = p.find_slot(m);
            if (auto id = p.row_page(p.slot)) {
                if (auto head = db->load_page_head(id)) {
                    if (head->is_index()) {
                        p.push_stack(head);
                        continue;
                    }
                    if (head->is_data()) {
                        return id;
                    }
                }
            }
            SDL_ASSERT(0);
            break;
        }
    }
    SDL_ASSERT(0);
    return{};
}

bool index_tree::key_less(key_mem x, key_mem y) const //FIXME: will be optimized or static-typed
{
    SDL_ASSERT(mem_size(x) == this->key_length);
    SDL_ASSERT(mem_size(y) == this->key_length);
    cluster_index const & cluster = index();
    for (size_t i = 0; i < cluster.size(); ++i) {
        size_t const sz = cluster.sub_key_length(i);
        sortorder const ord = cluster.order(i);
        x.second = x.first + sz;
        y.second = y.first + sz;
        switch (cluster[i].type) {
        case scalartype::t_int:
            if (auto px = index_key_cast<scalartype::t_int>(x)) {
            if (auto py = index_key_cast<scalartype::t_int>(y)) {
                if (ord == sortorder::DESC) {
                    std::swap(px, py);
                }
                if ((*px) < (*py)) return true;
                if ((*py) < (*px)) return false;
            }}
            break;
        case scalartype::t_bigint:
            if (auto px = index_key_cast<scalartype::t_bigint>(x)) {
            if (auto py = index_key_cast<scalartype::t_bigint>(y)) {
                if (ord == sortorder::DESC) {
                    std::swap(px, py);
                }
                if ((*px) < (*py)) return true;
                if ((*py) < (*px)) return false;
            }}
            break;
        case scalartype::t_uniqueidentifier:
            if (auto px = index_key_cast<scalartype::t_uniqueidentifier>(x)) {
            if (auto py = index_key_cast<scalartype::t_uniqueidentifier>(y)) {
                if (ord == sortorder::DESC) {
                    std::swap(px, py);
                }
                const int val = guid_t::compare(*px, *py);
                if (val < 0) return true;
                if (val > 0) return false;
            }}
            break;
        default:
            SDL_ASSERT(0);
            return false;
        }
        x.first = x.second;
        y.first = y.second;
    }
    return false; // keys are equal
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