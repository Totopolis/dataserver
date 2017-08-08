// database.h
//
#pragma once
#ifndef __SDL_SYSTEM_DATABASE_H__
#define __SDL_SYSTEM_DATABASE_H__

#include "dataserver/system/datatable.h"
#include "dataserver/system/database_cfg.h"
#include "dataserver/bpool/flag_type.h"

namespace sdl { namespace db {

class database: noncopyable {
public:
    enum class sysObj {
        //sysrscols = 3,
        sysrowsets = 5,
        sysschobjs = 34,
        syscolpars = 41,
        sysscalartypes = 50,
        sysidxstats = 54,
        sysiscols = 55,
        sysobjvalues = 60,
    };
    enum class sysPage {
        file_header = 0,
        PFS = 1,
        boot_page = 9,
    };
    template<class T> struct sysObj_t;
public:
    template<class T> using page_ptr = std::shared_ptr<T>;
    template<typename T> void load_page(page_ptr<T> &) const;
    template<typename T> void load_next(page_ptr<T> &) const;
    template<typename T> void load_prev(page_ptr<T> &) const;

    void load_page(page_ptr<sysallocunits> &) const;
    void load_page(page_ptr<pfs_page> &) const;

    using vector_sysallocunits_row = std::vector<sysallocunits_row const *>;
    using vector_page_head = std::vector<page_head const *>;
    using page_head_access = datatable::page_head_access;
    using shared_sysallocunits = std::shared_ptr<vector_sysallocunits_row>;
    using shared_page_head_access = std::shared_ptr<page_head_access>;
    using shared_usertables = std::shared_ptr<vector_shared_usertable>;
    using shared_datatables = std::shared_ptr<vector_shared_datatable>;

public: // for page_iterator

    template<typename T> static
    bool is_same(T const & p1, T const & p2) {
        if (p1 && p2) {
            A_STATIC_CHECK_TYPE(page_head const * const, p1->head);
            return p1->head == p2->head;
        }
        return p1 == p2; // both nullptr
    }
    template<typename T> static
    bool is_end(T const & p) {
        if (p) {
            A_STATIC_CHECK_TYPE(page_head const * const, p->head);
            SDL_ASSERT(p->head);
            return false;
        }
        return true;
    }
    template<typename T> static
    T const & dereference(T const & p) {
        SDL_ASSERT(!is_end(p));
        return p;
    }
private: 
    template<class pointer_type>
    class page_access_t : noncopyable {
        database const * const db;
    public:
        using value_type = typename pointer_type::element_type;
        using iterator = page_iterator<database const, pointer_type>;
        iterator begin() const {
            pointer_type p{};
            db->load_page(p);
            return iterator(db, std::move(p));
        }
        iterator end() const {
            return iterator(db);
        }
        explicit page_access_t(database const * p): db(p) {
            SDL_ASSERT(db);
        }
    };
    template<class T>
    using page_access = page_access_t<page_ptr<T>>;    

    class usertable_access : noncopyable {
        friend class database;
        shared_usertables tables;
        void init(shared_usertables const & value) {
            tables = value;
            SDL_ASSERT(tables);
        }
    public:
        using iterator = vector_shared_usertable::const_iterator;
        iterator begin() const {
            return tables->begin();
        }
        iterator end() const {
            return tables->end();
        }
    };
    class internal_access : noncopyable {
        friend class database;
        shared_usertables tables;
        void init(shared_usertables const & value) {
            tables = value;
            SDL_ASSERT(tables);
        }
    public:
        using iterator = vector_shared_usertable::const_iterator;
        iterator begin() const {
            return tables->begin();
        }
        iterator end() const {
            return tables->end();
        }
    };
    class datatable_access : noncopyable {
        friend class database;
        shared_datatables tables;
        void init(shared_datatables const & value) {
            tables = value;
            SDL_ASSERT(tables);
        }
    public:
        using iterator = vector_shared_datatable::const_iterator;
        iterator begin() const {
            return tables->begin();
        }
        iterator end() const {
            return tables->end();
        }
    };
    class iam_access {
        database const * const db;
        sysallocunits_row const * const alloc;
    public:
        using iterator = page_iterator<database const, shared_iam_page>;
        iam_access(database const * p, sysallocunits_row const * a)
            : db(p), alloc(a)
        {
            SDL_ASSERT(db && alloc);
        }
        iterator begin() const {
            SDL_ASSERT(db->is_allocated(alloc->data.pgfirstiam));
            return iterator(db, db->load_iam_page(alloc->data.pgfirstiam));
        }
        iterator end() const {
            return iterator(db);
        }
    };
private:
    class clustered_access: noncopyable {
        database const * const db;
        page_head const * const min_page;
        page_head const * const max_page;
    public:
        using iterator = page_iterator<clustered_access const, page_head const *>;
        clustered_access(database const * p, page_head const * _min, page_head const * _max)
            : db(p), min_page(_min), max_page(_max) {
            SDL_ASSERT(db && min_page && max_page);
            SDL_ASSERT(!min_page->data.prevPage);
            SDL_ASSERT(!max_page->data.nextPage);
            SDL_ASSERT(min_page->data.type == pageType::type::data);
            SDL_ASSERT(max_page->data.type == pageType::type::data);
        }
        iterator begin() const {
            page_head const * p = min_page;
            return iterator(this, std::move(p));
        }
        iterator end() const {
            return iterator(this);
        }
        template<class page_pos>
        page_head const * load_next_head(page_pos const & p) const {
            A_STATIC_CHECK_TYPE(page_head const *, p.first);
            return db->load_next_head(p.first);
        }
    private:
        friend iterator;
        static page_head const * dereference(page_head const * p) {
            return p;
        }
        void load_next(page_head const * & p) const {
            SDL_ASSERT(p);
            p = db->load_next_head(p);
        }
        void load_prev(page_head const * & p) const {
            p = p ? db->load_prev_head(p) : max_page;
        }
        static bool is_end(page_head const * const p) {
            return nullptr == p;
        }
    };
    class forward_access: noncopyable {
        database const * const db;
        page_head const * const head;
    public:
        using iterator = forward_iterator<forward_access const, page_head const *>;
        forward_access(database const * p, page_head const * h): db(p), head(h) {
            SDL_ASSERT(db && head);
            SDL_ASSERT(!head->data.prevPage);
            SDL_ASSERT(head->data.type == pageType::type::data);
        }
        iterator begin() const {
            page_head const * p = head;
            return iterator(this, std::move(p));
        }
        iterator end() const {
            return iterator(this);
        }
        template<class page_pos>
        page_head const * load_next_head(page_pos const & p) const {
            A_STATIC_CHECK_TYPE(page_head const *, p.first);
            return db->load_next_head(p.first);
        }
    private:
        friend iterator;
        static page_head const * dereference(page_head const * p) {
            return p;
        }
        void load_next(page_head const * & p) const {
            SDL_ASSERT(p);
            p = db->load_next_head(p);
        }
        static bool is_end(page_head const * const p) {
            return nullptr == p;
        }
    };
    class heap_access: noncopyable {
        vector_page_head const data;
    public:
        using iterator = vector_page_head::const_iterator;
        heap_access(database const *, vector_page_head && v): data(std::move(v)) {}
        iterator begin() const {
            return data.begin();
        }
        iterator end() const {
            return data.end();
        }
        template<class page_pos>
        page_head const * load_next_head(page_pos const & p) const {
            A_STATIC_CHECK_TYPE(size_t, p.second);
            size_t const i = p.second + 1;
            SDL_ASSERT(i <= data.size());
            if (i < data.size()) {
                return data[i];
            }
            return nullptr; 
        }
    };
private:
    template<class T> // T = clustered_access | heap_access
    class page_head_access_t final : public page_head_access {
        T _access;
    public:
        template<typename... Ts>
        page_head_access_t(Ts&&... params): _access(std::forward<Ts>(params)...) {}
    private:
        page_pos begin_page() const override {
            auto it = _access.begin();
            if (it != _access.end()) {
                return { *it, 0 };
            }
            return {};
        }
        void load_next(page_pos & p) const override {
            if ((p.first = _access.load_next_head(p)) != nullptr) {
                ++(p.second);
            }
            else {
                p = {};
            }
        }
    };
private:
    page_head const * sysallocunits_head() const;
    page_head const * load_sys_obj(sysObj) const;

    template<class T, class fun_type> static
    void for_row(page_access<T> const & obj, fun_type && fun) {
        for (auto & p : obj) {
            p->for_row(fun);
        }
    }
    template<class T, class fun_type> static
    typename T::const_pointer
    find_if(page_access<T> const & obj, fun_type && fun) {
        for (auto & p : obj) {
            if (auto found = p->find_if(fun)) {
                A_STATIC_CHECK_TYPE(typename T::const_pointer, found);
                return found;
            }
        }
        return nullptr;
    }   
    template<class fun_type> unique_datatable find_table_if(fun_type &&) const;
    template<class fun_type> unique_datatable find_internal_if(fun_type &&) const;
private:
    class pgroot_pgfirst {
        page_head const * m_pgroot = nullptr;  // root page of the index tree
        page_head const * m_pgfirst = nullptr; // first data page for this allocation unit
    public:
        pgroot_pgfirst() = default;
        pgroot_pgfirst(page_head const * p1, page_head const * p2): m_pgroot(p1), m_pgfirst(p2) {
            SDL_ASSERT(!m_pgroot == !m_pgfirst);
        }
        explicit operator bool() const {
            return m_pgroot && m_pgfirst;
        }
        page_head const * pgroot() const { return m_pgroot; }
        page_head const * pgfirst() const { return m_pgfirst; }
    };
    pgroot_pgfirst load_pg_index(schobj_id, pageType::type) const;
public:
    explicit database(const std::string & fname, database_cfg const & cfg = {});
    ~database();

    const std::string & filename() const;
    bool is_open() const;

    size_t page_count() const;
    size_t page_allocated() const;

    std::string dbi_dbname() const;
    bool use_page_bpool() const;
public:
    class scoped_thread_lock : noncopyable { // should be not used in main thread
        const database & m_db;
        const bpool::removef remove_id;
        SDL_DEBUG_HPP(const std::thread::id m_id;)
    public:
        explicit scoped_thread_lock(database const & db, bpool::removef = bpool::removef::false_);
        ~scoped_thread_lock();
    };
    std::thread::id init_thread_id() const;
    size_t unlock_thread(std::thread::id, bpool::removef) const; // returns blocks number
    size_t unlock_thread(bpool::removef) const; // returns blocks number
    size_t free_unlocked(bpool::decommitf) const; // returns blocks number

    bool unlock_page(pageIndex) const;
    bool unlock_page(pageFileID const &) const;
    bool unlock_page(page_head const *) const;

    page_head const * lock_page_fixed(pageIndex) const;
    page_head const * lock_page_fixed(pageFileID const &) const;
#if 0
    bpool::lock_page_head auto_lock_page(pageIndex) const;
    bpool::lock_page_head auto_lock_page(pageFileID const &) const;
#endif
    bool page_is_locked(pageIndex) const;
    bool page_is_locked(pageFileID const &) const;

    bool page_is_fixed(pageIndex) const;
    bool page_is_fixed(pageFileID const &) const;

    size_t pool_used_size() const;
    size_t pool_unused_size() const;
    size_t pool_free_size() const;
public:
    page_head const * load_page_head(pageIndex) const;
    page_head const * load_page_head(pageFileID const &) const;

    page_head const * load_next_head(page_head const *) const;
    page_head const * load_prev_head(page_head const *) const;

    page_head const * load_first_head(page_head const *) const; // first of page list 
    page_head const * load_last_head(page_head const *) const; // last of page list

    recordID load_next_record(recordID const &) const;
    recordID load_prev_record(recordID const &) const;

    using page_row = std::pair<page_head const *, row_head const *>;
    page_row load_page_row(recordID const &) const;

    void const * memory_offset(void const *) const; // diagnostic

    pageType get_pageType(pageFileID const &) const;
    pageFileID nextPageID(pageFileID const &) const;
    pageFileID prevPageID(pageFileID const &) const;
    bool is_pageType(pageFileID const &, pageType::type) const;

    page_ptr<bootpage> get_bootpage() const;
    page_ptr<fileheader> get_fileheader() const;
    page_ptr<datapage> get_datapage(pageIndex) const;
    page_ptr<sysallocunits> get_sysallocunits() const;
    page_ptr<pfs_page> get_pfs_page() const;

    const page_access<sysallocunits> _sysallocunits{this};
    const page_access<sysschobjs> _sysschobjs{this};
    const page_access<syscolpars> _syscolpars{this};
    const page_access<sysidxstats> _sysidxstats{this};
    const page_access<sysscalartypes> _sysscalartypes{this};
    const page_access<sysobjvalues> _sysobjvalues{this};
    const page_access<sysiscols> _sysiscols{ this };
    const page_access<sysrowsets> _sysrowsets{ this };
    const page_access<pfs_page> _pfs_page{ this };

    usertable_access _usertables;
    internal_access _internals; // INTERNAL_TABLE
    datatable_access _datatables;

    //_usertables
    unique_datatable find_table(const std::string & name) const;
    unique_datatable find_table(schobj_id) const;
    shared_usertable find_table_schema(schobj_id) const;

    //_internals
    unique_datatable find_internal(const std::string & name) const;
    unique_datatable find_internal(schobj_id) const;
    shared_usertable find_internal_schema(schobj_id) const;

    using spatial_root = std::pair<sysallocunits_row const *, sysidxstats_row const *>;
    spatial_root find_spatial_root(schobj_id) const;

    spatial_tree_idx find_spatial_tree(schobj_id) const;
    vector_sysidxstats_row index_for_table(schobj_id) const;

    shared_primary_key get_primary_key(schobj_id) const;
    shared_cluster_index get_cluster_index(shared_usertable const &) const;
    shared_cluster_index get_cluster_index(schobj_id) const; 
    page_head const * get_cluster_root(schobj_id) const; 
    
    shared_sysallocunits find_sysalloc(schobj_id, dataType::type) const;
    shared_page_head_access find_datapage(schobj_id, dataType::type, pageType::type) const;
    vector_mem_range_t var_data(row_head const *, size_t, scalartype::type) const;
    geo_mem get_geography(row_head const *, size_t) const;

    shared_iam_page load_iam_page(pageFileID const &) const;
    iam_access pgfirstiam(sysallocunits_row const * it) const { 
        return iam_access(this, it); 
    }
    bool is_allocated(pageFileID const &) const;
    bool is_allocated(page_head const *) const;

    // return type is const reference = const page_access<T> & 
    auto get_access(identity<sysallocunits>)  const -> decltype((_sysallocunits))   { return _sysallocunits; }
    auto get_access(identity<sysschobjs>)     const -> decltype((_sysschobjs))      { return _sysschobjs; }
    auto get_access(identity<syscolpars>)     const -> decltype((_syscolpars))      { return _syscolpars; }
    auto get_access(identity<sysidxstats>)    const -> decltype((_sysidxstats))     { return _sysidxstats; }
    auto get_access(identity<sysscalartypes>) const -> decltype((_sysscalartypes))  { return _sysscalartypes; }
    auto get_access(identity<sysobjvalues>)   const -> decltype((_sysobjvalues))    { return _sysobjvalues; }
    auto get_access(identity<sysiscols>)      const -> decltype((_sysiscols))       { return _sysiscols; }
    auto get_access(identity<sysrowsets>)     const -> decltype((_sysrowsets))      { return _sysrowsets; }
    auto get_access(identity<pfs_page>)       const -> decltype((_pfs_page))        { return _pfs_page; }
    auto get_access(identity<usertable>)      const -> decltype((_usertables))      { return _usertables; }
    auto get_access(identity<datatable>)      const -> decltype((_datatables))      { return _datatables; }

    template<class T> 
    auto get_access_t() const -> decltype(get_access(identity<T>())) {
        return this->get_access(identity<T>());
    }
    template<class T> // T = dbo_table
    std::unique_ptr<T> make_table() const {
        A_STATIC_CHECK_TYPE(schobj_id::type const, T::id);
        if (auto s = find_table_schema(_schobj_id(T::id))) {
            return std::make_unique<T>(this, s);
        }
        return {};
    }
public:
    using checksum_fun = std::function<bool(page_head const *)>; // called if checksum not valid
    break_or_continue scan_checksum(checksum_fun) const;
    break_or_continue scan_checksum() const;
private:
    template<class fun_type> void for_USER_TABLE(fun_type const &) const;
    template<class fun_type> void for_INTERNAL_TABLE(fun_type const &) const;
    template<class fun_type> void get_tables(vector_shared_usertable &, fun_type const &) const;

    shared_usertables get_usertables() const;
    shared_usertables get_internals() const;
    shared_datatables get_datatables() const;

    page_head const * load_page_head(sysPage) const;
    std::vector<page_head const *> load_page_list(page_head const *) const;

    sysidxstats_row const * find_spatial_type(const std::string & index_name, idxtype::type) const;
    sysidxstats_row const * find_spatial_idx(schobj_id) const;
    sysallocunits_row const * find_spatial_alloc(const std::string & index_name) const;

    shared_primary_key make_primary_key(schobj_id) const;
private:
    void init_database();
    void init_datatable(shared_usertable const &);
    using database_error = sdl_exception_t<database>;
    class shared_data;
    const std::unique_ptr<shared_data> m_data;
};

template<> struct database::sysObj_t<sysrowsets>      { static constexpr sysObj id = sysObj::sysrowsets; };
template<> struct database::sysObj_t<sysschobjs>      { static constexpr sysObj id = sysObj::sysschobjs; };
template<> struct database::sysObj_t<syscolpars>      { static constexpr sysObj id = sysObj::syscolpars; };
template<> struct database::sysObj_t<sysscalartypes>  { static constexpr sysObj id = sysObj::sysscalartypes; };
template<> struct database::sysObj_t<sysidxstats>     { static constexpr sysObj id = sysObj::sysidxstats; };
template<> struct database::sysObj_t<sysiscols>       { static constexpr sysObj id = sysObj::sysiscols; };
template<> struct database::sysObj_t<sysobjvalues>    { static constexpr sysObj id = sysObj::sysobjvalues; };

template <class T>
using decltype_table = decltype(std::declval<database>().make_table<T>());

template <class col>
using decltype_col = typename col::ret_type;

} // db
} // sdl

#include "dataserver/system/database.inl"

#endif // __SDL_SYSTEM_DATABASE_H__
