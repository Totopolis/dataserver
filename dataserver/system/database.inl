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

template<scalartype::type col_type>
vector_mem_range_t
database::var_data_t(row_head const * const row, size_t const i) const
{
    if (row->has_variable()) {
        const variable_array data(row);
        if (i >= data.size()) {
            SDL_ASSERT(!"wrong var_offset");
            return{};
        }
        const mem_range_t m = data.var_data(i);
        const size_t len = mem_size(m);
        if (!len) {
            return{}; // possible case (sdlSQL.mdf)
        }
        if (data.is_complex(i)) {
            // If length == 16 then we're dealing with a LOB pointer, otherwise it's a regular complex column
            if (len == sizeof(text_pointer)) { // 16 bytes
                auto const tp = reinterpret_cast<text_pointer const *>(m.first);
                if ((col_type == scalartype::t_text) || 
                    (col_type == scalartype::t_ntext)) {
                    return text_pointer_data(this, tp).detach();
                }
            }
            else {
                const auto type = data.var_complextype(i);
                SDL_ASSERT(type != complextype::none);
                if (type == complextype::row_overflow) {
                    if (len == sizeof(overflow_page)) { // 24 bytes
                        auto const overflow = reinterpret_cast<overflow_page const *>(m.first);
                        SDL_ASSERT(overflow->type == type);
                        if (col_type == scalartype::t_varchar) {
                            return varchar_overflow_page(this, overflow).detach();
                        }
                    }
                }
                else if (type == complextype::blob_inline_root) {
                    if (len == sizeof(overflow_page)) { // 24 bytes
                        auto const overflow = reinterpret_cast<overflow_page const *>(m.first);
                        SDL_ASSERT(overflow->type == type);
                        if (col_type == scalartype::t_geography) {
                            varchar_overflow_page varchar(this, overflow);
                            SDL_ASSERT(varchar.length() == overflow->length);
                            return varchar.detach();
                        }
                    }
                    if (len > sizeof(overflow_page)) { // 24 bytes + 12 bytes * link_count 
                        SDL_ASSERT(!((len - sizeof(overflow_page)) % sizeof(overflow_link)));
                        if (col_type == scalartype::t_geography) {
                            auto const page = reinterpret_cast<overflow_page const *>(m.first);
                            auto const link = reinterpret_cast<overflow_link const *>(page + 1);
                            size_t const link_count = (len - sizeof(overflow_page)) / sizeof(overflow_link);
                            varchar_overflow_page varchar(this, page);
                            SDL_ASSERT(varchar.length() == page->length);
                            auto total_length = page->length;
                            for (size_t i = 0; i < link_count; ++i) {
                                auto const nextlink = link + i;
                                varchar_overflow_link next(this, page, nextlink);
                                SDL_ASSERT(next.length() == (nextlink->size - total_length));
                                append(varchar.data(), next.detach());
                                SDL_ASSERT(total_length < nextlink->size);
                                total_length = nextlink->size;
                                SDL_ASSERT(mem_size_n(varchar.data()) == total_length);
                            }
                            throw_error_if_not<database_error>(
                                mem_size_n(varchar.data()) == total_length,
                                "bad varchar_overflow_page"); //FIXME: to be tested
                            return varchar.detach();
                        }
                    }
                }
            }
            if (col_type == scalartype::t_varbinary) {
                return { m };
            }
            SDL_ASSERT(!"unknown data type");
            return { m };
        }
        else { // in-row-data
            return { m };
        }
    }
    SDL_ASSERT(0);
    return {};
}

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_INL__
