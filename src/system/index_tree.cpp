// index_tree.cpp
//
#include "common/common.h"
#include "index_tree.h"
#include "database.h"

namespace sdl { namespace db { 
    
index_tree::index_tree(database * p, unique_cluster_index && h)
: db(p), key_length(h->key_length())
{
    cluster = std::move(h);
    SDL_ASSERT(db && cluster && cluster->root);
    SDL_ASSERT(cluster->root->data.type == db::pageType::type::index);  
    SDL_ASSERT(!(cluster->root->data.prevPage));
    SDL_ASSERT(!(cluster->root->data.nextPage));
    SDL_ASSERT(root()->data.pminlen == key_length + 7);
    SDL_ASSERT(key_length);
}

index_tree::index_access
index_tree::get_begin()
{
    return index_access(this, cluster->root);
}

index_tree::index_access
index_tree::get_end()
{
    return index_access(this, cluster->root, slot_array::size(cluster->root));
}

bool index_tree::is_end(index_access const & p)
{
    SDL_ASSERT(p.slot_index <= slot_array::size(p.head));
    return p.slot_index == slot_array::size(p.head);
}

bool index_tree::is_begin(index_access const & p)
{
    if (!p.slot_index) {
        return !(p.head->data.prevPage);
    }
    return false;
}

void index_tree::load_next(index_access & p)
{
    SDL_ASSERT(!is_end(p));
    if (++p.slot_index == slot_array::size(p.head)) {
        if (auto h = db->load_next_head(p.head)) {
            p.slot_index = 0;
            p.head = h;
        }
    }
}

void index_tree::load_prev(index_access & p)
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

//--------------------------------------------------------------------------

index_tree::index_access::mem_type
index_tree::index_access::get() const
{
    using T = index_page_row_t<char>;
    using index_page = datapage_t<T>;
    SDL_ASSERT(head->data.pminlen == parent->key_length + index_row_head_size);
    SDL_ASSERT(head->data.pminlen >= sizeof(T));
    auto row = index_page(head)[slot_index];
    const char * const p1 = &(row->data.key);
    const char * const p2 = p1 + parent->key_length;
    SDL_ASSERT(p1 < p2);
    const pageFileID & page = * reinterpret_cast<const pageFileID *>(p2);
    return { mem_range_t(p1, p2), page };
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
                    /*using T1 = index_tree_t<scalartype::t_int>;
                    using T2 = index_tree_t<scalartype::t_bigint>;
                    A_STATIC_ASSERT_TYPE(int32, T1::key_type);
                    A_STATIC_ASSERT_TYPE(int64, T2::key_type);
                    if (0) {
                        index_tree_t<scalartype::t_int> tree(nullptr, nullptr);
                        for (auto row : tree) {
                            (void)row;
                        }
                    }*/
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG