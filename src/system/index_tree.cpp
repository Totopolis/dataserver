// index_tree.cpp
//
#include "common/common.h"
#include "index_tree.h"
#include "database.h"

namespace sdl { namespace db {

index_tree_base::index_tree_base(database * p, page_head const * h)
    : db(p), root(h)
{
    SDL_ASSERT(db && root);
    SDL_ASSERT(root->data.type == db::pageType::type::index);  
    SDL_ASSERT(!(root->data.prevPage));
    SDL_ASSERT(!(root->data.nextPage));
}

index_tree_base::index_access
index_tree_base::get_begin()
{
    return index_access(this, root);
}

index_tree_base::index_access
index_tree_base::get_end()
{
    return index_access(this, root, slot_array::size(root));
}

bool index_tree_base::is_end(index_access const & p)
{
    SDL_ASSERT(p.slot_index <= slot_array::size(p.head));
    return p.slot_index == slot_array::size(p.head);
}

bool index_tree_base::is_begin(index_access const & p)
{
    if (!p.slot_index) {
        return !(p.head->data.prevPage);
    }
    return false;
}

void index_tree_base::load_next(index_access & p)
{
    SDL_ASSERT(!is_end(p));
    if (++p.slot_index == slot_array::size(p.head)) {
        if (auto h = db->load_next_head(p.head)) {
            p.slot_index = 0;
            p.head = h;
        }
    }
}

void index_tree_base::load_prev(index_access & p)
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

#if 0
pageFileID const &
index_tree::dereference(index_access const & p) const
{
    if (key_type == scalartype::t_int) {
        using row_type = index_page_row_t<index_key<scalartype::t_int>::type>;
        return p.dereference<row_type>()->data.page;
    }
    else if (key_type == scalartype::t_bigint) {
        using row_type = index_page_row_t<index_key<scalartype::t_bigint>::type>;
        return p.dereference<row_type>()->data.page;
    }
    SDL_ASSERT(0);
    static const pageFileID null{};
    return null;
}
#endif

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
                    using T1 = index_tree_t<scalartype::t_int>;
                    using T2 = index_tree_t<scalartype::t_bigint>;
                    A_STATIC_ASSERT_TYPE(int32, T1::key_type);
                    A_STATIC_ASSERT_TYPE(int64, T2::key_type);
                    if (0) {
                        index_tree_t<scalartype::t_int> tree(nullptr, nullptr);
                        for (auto row : tree) {
                            (void)row;
                        }
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG