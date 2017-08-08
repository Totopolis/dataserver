// database.inl
//
#pragma once
#ifndef __SDL_SYSTEM_DATABASE_INL__
#define __SDL_SYSTEM_DATABASE_INL__

namespace sdl { namespace db {

template<class T> inline
auto get_access(database const & db) -> decltype(db.get_access_t<T>()) {
    return db.get_access_t<T>();
}

inline const char * page_name_t(identity<datatable>) { return "datatable"; }
inline const char * page_name_t(identity<usertable>) { return "usertable"; }

template<class T> inline const char * page_name_t(identity<T>) { return T::name(); }
template<class T> inline const char * page_name() { 
    return page_name_t(identity<T>());
}

#if SDL_USE_BPOOL
inline bool database::unlock_page(pageFileID const & id) const {
    if (id) {
        return this->unlock_page(id.pageId);
    }
    return false;
}

inline bool database::unlock_page(page_head const * const p) const {
    if (p) {
        return this->unlock_page(p->data.pageId.pageId);
    }
    return false;
}

inline page_head const *
database::lock_page_fixed(pageFileID const & id) const {
    if (id) {
        return this->lock_page_fixed(id.pageId);
    }
    return nullptr;
}

inline bool database::page_is_locked(pageFileID const & id) const {
    if (id) {
        return this->page_is_locked(id.pageId);
    }
    return false;
}

inline bool database::page_is_fixed(pageFileID const & id) const {
    if (id) {
        return this->page_is_fixed(id.pageId);
    }
    return false;
}

#endif // SDL_USE_BPOOL

inline page_head const *
database::load_page_head(pageFileID const & id) const {
    if (id) {
        return this->load_page_head(id.pageId);
    }
    return nullptr;
}

inline page_head const * 
database::load_page_head(sysPage const i) const
{
    return this->load_page_head(static_cast<pageIndex::value_type>(i));
}
inline void database::load_page(page_ptr<sysallocunits> & p) const {
    p = get_sysallocunits();
}
inline void database::load_page(page_ptr<pfs_page> & p) const {
    p = get_pfs_page();
}

template<typename T> inline
void database::load_page(page_ptr<T> & p) const {
    if (auto h = load_sys_obj(database::sysObj_t<T>::id)) {
        reset_new(p, h);
    }
    else {
        SDL_ASSERT(0);
        p.reset();
    }
}

template<class T> inline
void database::load_next(page_ptr<T> & p) const {
    if (p) {
        A_STATIC_CHECK_TYPE(page_head const * const, p->head);
        if (auto h = this->load_next_head(p->head)) {
            A_STATIC_CHECK_TYPE(page_head const *, h);
            reset_new(p, h);
        }
        else {
            p.reset();
        }
    }
    else {
        SDL_ASSERT(0);
    }
}

template<class T> inline
void database::load_prev(page_ptr<T> & p) const {
    if (p) {
        A_STATIC_CHECK_TYPE(page_head const * const, p->head);
        if (auto h = this->load_prev_head(p->head)) {
            A_STATIC_CHECK_TYPE(page_head const *, h);
            reset_new(p, h);
        }
        else {
            SDL_ASSERT(0);
            p.reset();
        }
    }
    else {
        SDL_ASSERT(0);
    }
}

inline constexpr bool database::use_page_bpool() {
    return SDL_USE_BPOOL != 0;
}

#if SDL_USE_BPOOL
inline void const * database::memory_offset(void const * p) const { // diagnostic
    return p;
}
#endif

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_INL__
