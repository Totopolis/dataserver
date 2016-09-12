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

inline page_head const * 
database::load_page_head(sysPage const i) const
{
    return load_page_head(static_cast<pageIndex::value_type>(i));
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

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_INL__
