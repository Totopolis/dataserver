// primary_key.cpp
//
#include "common/common.h"
#include "primary_key.h"

namespace sdl { namespace db { namespace {

struct key_size_count {
    size_t & result;
    key_size_count(size_t & s) : result(s){}
    template<class T> // T = index_key_t<>
    void operator()(T) {
        result += sizeof(typename T::type);
    }
};

} // namespace

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
    SDL_ASSERT(root && root->is_index());
    SDL_ASSERT(!col_index.empty());
    SDL_ASSERT(col_index.size() == col_ord.size());
    SDL_ASSERT(schema.get());
    init_key_length();
}

void cluster_index::init_key_length()
{
    SDL_ASSERT(!m_key_length);
    m_sub_key_length.resize(col_index.size());
    for (size_t i = 0; i < col_index.size(); ++i) {
        size_t len = 0;
        case_index_key((*this)[i].type, key_size_count(len));
        m_key_length += len;
        m_sub_key_length[i] = len;
    }
    SDL_ASSERT(m_key_length);
}

cluster_index::column_ref
cluster_index::operator[](size_t i) const
{
    SDL_ASSERT(i < size());
    return (*schema)[col_index[i]];
}

sortorder cluster_index::order(size_t i) const
{
    SDL_ASSERT(i < size());
    return col_ord[i];
}

} // db
} // sdl

