// primary_key.cpp
//
#include "common/common.h"
#include "primary_key.h"

namespace sdl { namespace db {

primary_key::primary_key(page_head const * p, sysidxstats_row const * stat,
    colpars && _c, scalars && _s, orders && _o, schobj_id id)
    : root(p), idxstat(stat)
    , colpar(std::move(_c))
    , scalar(std::move(_s))
    , order(std::move(_o))
    , table_id(id)
{
    SDL_ASSERT(!colpar.empty());
    SDL_ASSERT(colpar.size() == scalar.size());
    SDL_ASSERT(colpar.size() == order.size());
    SDL_ASSERT(root && idxstat);
    SDL_ASSERT(root->is_index() || root->is_data());
    SDL_ASSERT(slot_array::size(root));
    SDL_ASSERT(idxstat->is_clustered());
    SDL_ASSERT(idxstat->IsPrimaryKey() || idxstat->IsUnique());
    SDL_ASSERT(!this->name().empty());
}

cluster_index::cluster_index(
    shared_primary_key const & p,
    shared_usertable const & s,
    column_index && ci)
    : primary(p)
    , m_schema(s)
    , m_index(std::move(ci))
{
    SDL_ASSERT(primary && m_schema);
    SDL_ASSERT(primary->root->is_index());
    SDL_ASSERT(primary->order.size() == m_index.size());
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

