// index_tree.cpp
//
#include "common/common.h"
#include "index_tree.h"
#include "database.h"
#include "page_info.h"

namespace sdl { namespace db { 

index_tree::index_access::index_access(index_tree const * t, size_t const i)
    : tree(t)
    , head(t->root())
    , slot(i)
{
    SDL_ASSERT(tree && head);
    SDL_ASSERT(head->data.pminlen == tree->key_length + index_row_head_size);
    SDL_ASSERT(slot <= slot_array::size(head));
}

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

index_tree::index_access
index_tree::get_begin()
{
    return index_access(this);
}

index_tree::index_access
index_tree::get_end()
{
    return index_access(this, slot_array::size(root()));
}

bool index_tree::is_begin(index_access const & p) const
{
    if ((p.head == root()) && !p.slot) {
        SDL_ASSERT(p.stack.empty());
        return true;
    }
    return false;
}

bool index_tree::is_end(index_access const & p) const
{
    if ((p.head == root()) && (p.slot == slot_array::size(p.head))) {
        SDL_ASSERT(p.stack.empty());
        return true;
    }
    return false;
}

void index_tree::load_next(index_access & p)
{
    SDL_ASSERT(!is_end(p));
    SDL_ASSERT(p.head->is_index());
    page_head const * const next = db->load_page_head(p.row_page());
    if (next) {
        if (next->is_index()) { // intermediate level
            SDL_ASSERT(p.slot < slot_array::size(p.head));
            p.stack.emplace_back(p.head, p.slot);
            p.head = next;
            p.slot = 0;
            SDL_ASSERT(slot_array::size(p.head));
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
                SDL_ASSERT(is_end(p));
            }
        }
    }
    else {
        throw_error<index_tree_error>("bad index");
    }
}

index_tree::page_stack const &
index_tree::get_stack(iterator const & it) const
{
    return it.current.get_stack();
}

size_t index_tree::get_slot(iterator const & it) const
{
    return it.current.get_slot();
}

namespace {

    struct type_key_fun
    {
        using column = usertable::column;

        std::string & result;
        mem_range_t data;
        column const & col;

        type_key_fun(std::string & s, mem_range_t const & m, column const & c)
            : result(s), data(m), col(c) {}

        template<class T> // T = index_key_t
        void operator()(T) {
            if (auto pv = scalartype_cast<typename T::type, T::value>(data, col)) {
                result = to_string::type(*pv);
            }
        }
    };
}

std::string index_tree::type_key(row_mem_type const & row) const
{
    SDL_ASSERT(mem_size(row.first));
    std::string result("[");
    mem_range_t m = row.first;
    cluster_index const & cluster = index();
    for (size_t i = 0; i < cluster.size(); ++i) {
        auto & col = cluster[i];
        m.second = m.first + cluster.sub_key_length(i);
        std::string s;
        case_index_key(col.type, type_key_fun(s, m, col));
        if (i) result += ",";
        result += std::move(s);
        m.first = m.second;
    }
    result += "]";
    return result;
}

index_tree::row_mem_type
index_tree::index_access::row_data() const
{
    auto const row = datapage_t<index_page_row_char>(this->head)[this->slot];
    const char * const p1 = &(row->data.key);
    const char * const p2 = p1 + tree->key_length;
    SDL_ASSERT(p1 < p2);
    const pageFileID & page = * reinterpret_cast<const pageFileID *>(p2);
    return { mem_range_t(p1, p2), page };
}

pageFileID const & index_tree::index_access::row_page() const
{
    auto const row = datapage_t<index_page_row_char>(this->head)[this->slot];
    const char * const p1 = &(row->data.key);
    const char * const p2 = p1 + tree->key_length;
    SDL_ASSERT(p1 < p2);
    return * reinterpret_cast<const pageFileID *>(p2);
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