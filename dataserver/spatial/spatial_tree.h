// spatial_tree.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TREE_H__
#define __SDL_SYSTEM_SPATIAL_TREE_H__

#include "spatial_tree_t.h"

namespace sdl { namespace db {

template<typename pk0_type> using shared_spatial_tree_t = std::shared_ptr<spatial_tree_t<pk0_type> >;
template<typename pk0_type> using unique_spatial_tree_t = std::unique_ptr<spatial_tree_t<pk0_type> >;

struct spatial_tree_idx {
    page_head const * pgroot = nullptr;
    sysidxstats_row const * idx = nullptr;
    explicit operator bool() const {
        return pgroot && idx;
    }
};

namespace bigint {
    using spatial_tree = spatial_tree_t<int64>;
    using unique_spatial_tree = std::unique_ptr<spatial_tree>;
}

class spatial_tree : noncopyable {
    template<typename pk0_type>
    using tree_type = spatial_tree_t<pk0_type>;
    using unique_tree = std::unique_ptr<spatial_tree_base const>;
public:
    const scalartype::type pk0_scalartype = scalartype::t_none;
    spatial_tree() = default;
    template<typename pk0_type, typename... Ts>
    spatial_tree(identity<pk0_type>, Ts&&... params)
        : pk0_scalartype(key_to_scalartype<pk0_type>::value)
        , m_tree(new tree_type<pk0_type>(std::forward<Ts>(params)...)) {
        SDL_ASSERT(m_tree);
        SDL_ASSERT(pk0_scalartype != scalartype::t_none);
        SDL_ASSERT(this->cast<pk0_type>());
    }
    template<typename pk0_type, typename... Ts>
    static spatial_tree make(Ts&&... params) {
        return spatial_tree(identity<pk0_type>(), std::forward<Ts>(params)...);
    }
    spatial_tree(spatial_tree && src)
        : pk0_scalartype(src.pk0_scalartype)
        , m_tree(std::move(src.m_tree))
    {}
    spatial_tree & operator=(spatial_tree && src) {
        SDL_ASSERT(pk0_scalartype == src.pk0_scalartype);
        m_tree.swap(src.m_tree);
        return *this;
    }
    explicit operator bool() const {
        return !!m_tree;
    }
    spatial_tree_base const * operator ->() const {
        return m_tree.get();
    }
    template<typename pk0_type> spatial_tree_t<pk0_type> const * cast() const && = delete;
    template<typename pk0_type> spatial_tree_t<pk0_type> const * cast() const & {
        SDL_ASSERT(pk0_scalartype == key_to_scalartype<pk0_type>::value);
        return static_cast<tree_type<pk0_type> const *>(m_tree.get());
    }
private:
    unique_tree m_tree;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TREE_H__
