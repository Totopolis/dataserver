// primary_key.cpp
//
#include "common/common.h"
#include "primary_key.h"

namespace sdl { namespace db {

primary_key::primary_key(page_head const * p, colpars && _c, scalars && _s, orders && _o)
    : root(p)
    , colpar(std::move(_c))
    , scalar(std::move(_s))
    , order(std::move(_o))
{
    SDL_ASSERT(!colpar.empty());
    SDL_ASSERT(colpar.size() == scalar.size());
    SDL_ASSERT(colpar.size() == order.size());
    SDL_ASSERT(root);
    SDL_ASSERT(root->is_index() || root->is_data());
    SDL_ASSERT(slot_array::size(root));
}

cluster_index::cluster_index(page_head const * p,
    column_index && _c,
    column_order && _o,
    shared_usertable const & _s)
    : root(p)
    , col_index(std::move(_c))
    , col_ord(std::move(_o))
    , schema(_s)
{
    SDL_ASSERT(schema);
    SDL_ASSERT(root && root->is_index());
    SDL_ASSERT(col_ord.size() == size());
    SDL_ASSERT(size());

    m_sub_key_length.resize(size());
    for (size_t i = 0; i < size(); ++i) {
        const size_t len = index_key_size((*this)[i].type);
        m_key_length += len;
        m_sub_key_length[i] = len;
    }
    SDL_ASSERT(m_key_length);
}

} // db
} // sdl

