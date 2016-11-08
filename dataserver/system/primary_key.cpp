// primary_key.cpp
//
#include "dataserver/common/common.h"
#include "dataserver/system/primary_key.h"

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
    SDL_ASSERT(primary->order.size() == m_index.size());
    SDL_ASSERT(this->size());

    m_sub_key_length.resize(size());
    for (size_t i = 0, end = size(); i < end; ++i) {
        const size_t len = (*this)[i].fixed_size();
        m_sub_key_length[i] = len;
        m_key_length += len;
    }
    SDL_ASSERT(m_key_length);
}

} // db
} // sdl

