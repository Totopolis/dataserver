// page_bpool.inl
//
#pragma once
#ifndef __SDL_BPOOL_PAGE_BPOOL_INL__
#define __SDL_BPOOL_PAGE_BPOOL_INL__

namespace sdl { namespace db { namespace bpool {

inline thread_id_t::size_bool
thread_id_t::insert(id_type const id)
{
    return algo::unique_insertion_distance(m_data, id);
}

inline size_t thread_id_t::find(id_type const id) const
{
    return std::distance(m_data.begin(), algo::binary_find(m_data, id));
}

inline bool thread_id_t::erase(id_type const id)
{
    auto it = algo::binary_find(m_data, id);
    if (it != m_data.end()) {
        m_data.erase(it);
        return true;
    }
    return false;
}

}}} // sdl

#endif // __SDL_BPOOL_PAGE_BPOOL_INL__
