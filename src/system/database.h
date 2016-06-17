// database.h
//
#pragma once
#ifndef __SDL_SYSTEM_DATABASE_H__
#define __SDL_SYSTEM_DATABASE_H__

#include "datatable.h"

namespace sdl { namespace db {

class database: noncopyable
{
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

    template<typename T> void load_page(page_ptr<T> &);
    template<typename T> void load_next(page_ptr<T> &);
    template<typename T> void load_prev(page_ptr<T> &);

    void load_page(page_ptr<sysallocunits> &);
    void load_page(page_ptr<pfs_page> &);

    using vector_sysallocunits_row = std::vector<sysallocunits_row const *>;
    using vector_page_head = std::vector<page_head const *>;

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
        database * const db;
    public:
        using value_type = typename pointer_type::element_type;
        using iterator = page_iterator<database, pointer_type>;
        iterator begin() {
            pointer_type p{};
            db->load_page(p);
            return iterator(db, std::move(p));
        }
        iterator end() {
            return iterator(db);
        }
        explicit page_access_t(database * p): db(p) {
            SDL_ASSERT(db);
        }
    };
    template<class T>
    using page_access = page_access_t<page_ptr<T>>;    

    class usertable_access : noncopyable {
        database * const db;
        vector_shared_usertable const & data() {
            return db->get_usertables();
        }
    public:
        using iterator = vector_shared_usertable::const_iterator;
        iterator begin() {
            return data().begin();
        }
        iterator end() {
            return data().end();
        }
        explicit usertable_access(database * p): db(p) {
            SDL_ASSERT(db);
        }
    };
    class internal_access : noncopyable {
        database * const db;
        vector_shared_usertable const & data() {
            return db->get_internals();
        }
    public:
        using iterator = vector_shared_usertable::const_iterator;
        iterator begin() {
            return data().begin();
        }
        iterator end() {
            return data().end();
        }
        explicit internal_access(database * p): db(p) {
            SDL_ASSERT(db);
        }
    };
    class datatable_access : noncopyable {
        database * const db;
        vector_shared_datatable const & data() {
            return db->get_datatable();
        }
    public:
        using iterator = vector_shared_datatable::const_iterator;
        iterator begin() {
            return data().begin();
        }
        iterator end() {
            return data().end();
        }
        explicit datatable_access(database * p): db(p) {
            SDL_ASSERT(db);
        }
    };
    class iam_access {
        database * const db;
        sysallocunits_row const * const alloc;
    public:
        using iterator = page_iterator<database, shared_iam_page>;
        iam_access(database * p, sysallocunits_row const * a)
            : db(p), alloc(a)
        {
            SDL_ASSERT(db && alloc);
        }
        iterator begin() {
            return iterator(db, db->load_iam_page(alloc->data.pgfirstiam));
        }
        iterator end() {
            return iterator(db);
        }
    };
private:
    class clustered_access: noncopyable {
        database * const db;
        page_head const * const min_page;
        page_head const * const max_page;
        size_t m_size = 0;
    public:
        using iterator = page_iterator<clustered_access, page_head const *>;
        clustered_access(database * p, page_head const * _min, page_head const * _max)
            : db(p), min_page(_min), max_page(_max) {
            SDL_ASSERT(db && min_page && max_page);
            SDL_ASSERT(!min_page->data.prevPage);
            SDL_ASSERT(!max_page->data.nextPage);
            SDL_ASSERT(min_page->data.type == pageType::type::data);
            SDL_ASSERT(max_page->data.type == pageType::type::data);
        }
        iterator begin() {
            page_head const * p = min_page;
            return iterator(this, std::move(p));
        }
        iterator end() {
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
        void load_next(page_head const * & p) {
            SDL_ASSERT(p);
            p = db->load_next_head(p);
        }
        void load_prev(page_head const * & p) {
            p = p ? db->load_prev_head(p) : max_page;
        }
        static bool is_end(page_head const * const p) {
            return nullptr == p;
        }
    };
    class forward_access: noncopyable {
        database * const db;
        page_head const * const head;
        size_t m_size = 0;
    public:
        using iterator = forward_iterator<forward_access, page_head const *>;
        forward_access(database * p, page_head const * h): db(p), head(h) {
            SDL_ASSERT(db && head);
            SDL_ASSERT(!head->data.prevPage);
            SDL_ASSERT(head->data.type == pageType::type::data);
        }
        iterator begin() {
            page_head const * p = head;
            return iterator(this, std::move(p));
        }
        iterator end() {
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
        void load_next(page_head const * & p) {
            SDL_ASSERT(p);
            p = db->load_next_head(p);
        }
        static bool is_end(page_head const * const p) {
            return nullptr == p;
        }
    };
    class heap_access: noncopyable {
        database * const db;
        vector_page_head const data;
    public:
        using iterator = vector_page_head::const_iterator;
        heap_access(database * p, vector_page_head && v): db(p), data(std::move(v)) {
            SDL_ASSERT(db);
        }
        iterator begin() {
            return data.begin();
        }
        iterator end() {
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
public:
    using page_head_access = datatable::page_head_access;
private:
    template<class T> // T = clustered_access | heap_access
    class page_head_access_t final : public page_head_access {
        T _access;
    public:
        template<typename... Ts>
        page_head_access_t(Ts&&... params): _access(std::forward<Ts>(params)...) {}
    private:
        page_pos begin_page() override {
            auto it = _access.begin();
            if (it != _access.end()) {
                return { *it, 0 };
            }
            return {};
        }
        void load_next(page_pos & p) override {
            if ((p.first = _access.load_next_head(p)) != nullptr) {
                ++(p.second);
            }
            else {
                p = {};
            }
        }
    };
private:
    page_head const * sysallocunits_head();
    page_head const * load_sys_obj(sysObj);

    template<class T, class fun_type> static
    void for_row(page_access<T> & obj, fun_type fun) {
        for (auto & p : obj) {
            p->for_row(fun);
        }
    }
    template<class T, class fun_type> static
    typename T::const_pointer
    find_if(page_access<T> & obj, fun_type fun) {
        for (auto & p : obj) {
            if (auto found = p->find_if(fun)) {
                A_STATIC_CHECK_TYPE(typename T::const_pointer, found);
                return found;
            }
        }
        return nullptr;
    }   
    template<class fun_type> unique_datatable find_table_if(fun_type);
    template<class fun_type> unique_datatable find_internal_if(fun_type);
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
    pgroot_pgfirst load_pg_index(schobj_id, pageType::type); 
public:
    explicit database(const std::string & fname);
    ~database();

    const std::string & filename() const;

    bool is_open() const;

    size_t page_count() const;

    page_head const * load_page_head(pageIndex);
    page_head const * load_page_head(pageFileID const &);

    page_head const * load_next_head(page_head const *);
    page_head const * load_prev_head(page_head const *);

    page_head const * load_first_head(page_head const *); // first of page list 
    page_head const * load_last_head(page_head const *); // last of page list

    using page_row = std::pair<page_head const *, row_head const *>;
    page_row load_page_row(recordID const &);

    void const * start_address() const; // diagnostic
    void const * memory_offset(void const *) const; // diagnostic

    pageType get_pageType(pageFileID const &);
    pageFileID nextPageID(pageFileID const &);
    pageFileID prevPageID(pageFileID const &);
    bool is_pageType(pageFileID const &, pageType::type);

    page_ptr<bootpage> get_bootpage();
    page_ptr<fileheader> get_fileheader();
    page_ptr<datapage> get_datapage(pageIndex);
    page_ptr<sysallocunits> get_sysallocunits();
    page_ptr<pfs_page> get_pfs_page();

    page_access<sysallocunits> _sysallocunits{this};
    page_access<sysschobjs> _sysschobjs{this};
    page_access<syscolpars> _syscolpars{this};
    page_access<sysidxstats> _sysidxstats{this};
    page_access<sysscalartypes> _sysscalartypes{this};
    page_access<sysobjvalues> _sysobjvalues{this};
    page_access<sysiscols> _sysiscols{ this };
    page_access<sysrowsets> _sysrowsets{ this };
    page_access<pfs_page> _pfs_page{ this };

    usertable_access _usertables{this};
    internal_access _internals{this}; // INTERNAL_TABLE
    datatable_access _datatables{this};

    //_usertables
    unique_datatable find_table(const std::string & name);
    unique_datatable find_table(schobj_id);
    shared_usertable find_table_schema(schobj_id);

    //_internals
    unique_datatable find_internal(const std::string & name);
    unique_datatable find_internal(schobj_id);
    shared_usertable find_internal_schema(schobj_id);

    void find_spatial_root(schobj_id, const std::string & name);
    vector_sysidxstats_row index_for_table(schobj_id);

    shared_primary_key get_primary_key(schobj_id);
    shared_cluster_index get_cluster_index(shared_usertable const &); 
    shared_cluster_index get_cluster_index(schobj_id); 
    page_head const * get_cluster_root(schobj_id); 
    
    vector_sysallocunits_row const & find_sysalloc(schobj_id, dataType::type);
    page_head_access & find_datapage(schobj_id, dataType::type, pageType::type);
    vector_mem_range_t var_data(row_head const *, size_t, scalartype::type);
    
    shared_iam_page load_iam_page(pageFileID const &);

    iam_access pgfirstiam(sysallocunits_row const * it) { 
        return iam_access(this, it); 
    }
    bool is_allocated(pageFileID const &);
    bool is_allocated(page_head const *);

    auto get_access(identity<sysallocunits>)  -> decltype((_sysallocunits))   { return _sysallocunits; }
    auto get_access(identity<sysschobjs>)     -> decltype((_sysschobjs))      { return _sysschobjs; }
    auto get_access(identity<syscolpars>)     -> decltype((_syscolpars))      { return _syscolpars; }
    auto get_access(identity<sysidxstats>)    -> decltype((_sysidxstats))     { return _sysidxstats; }
    auto get_access(identity<sysscalartypes>) -> decltype((_sysscalartypes))  { return _sysscalartypes; }
    auto get_access(identity<sysobjvalues>)   -> decltype((_sysobjvalues))    { return _sysobjvalues; }
    auto get_access(identity<sysiscols>)      -> decltype((_sysiscols))       { return _sysiscols; }
    auto get_access(identity<sysrowsets>)     -> decltype((_sysrowsets))      { return _sysrowsets; }
    auto get_access(identity<pfs_page>)       -> decltype((_pfs_page))        { return _pfs_page; }
    auto get_access(identity<usertable>)      -> decltype((_usertables))      { return _usertables; }
    auto get_access(identity<datatable>)      -> decltype((_datatables))      { return _datatables; }

    template<class T> 
    auto get_access_t() -> decltype(get_access(identity<T>())) {
        return this->get_access(identity<T>());
    }
    template<class T> // T = dbo_table
    std::unique_ptr<T> make_table() {
        if (auto s = find_table_schema(_schobj_id(T::id))) {
            return sdl::make_unique<T>(this, s);
        }
        return {};
    }
private:
    template<class fun_type> void for_USER_TABLE(fun_type);
    template<class fun_type> void for_INTERNAL_TABLE(fun_type);
    template<class fun_type> void get_tables(vector_shared_usertable &, fun_type);

    vector_shared_usertable const & get_usertables();
    vector_shared_usertable const & get_internals();
    vector_shared_datatable const & get_datatable();

    page_head const * load_page_head(sysPage);
    std::vector<page_head const *> load_page_list(page_head const *);

    sysidxstats_row const * find_spatial(const std::string & name, idxtype::type);
private:
    database(const database&) = delete;
    const database& operator=(const database&) = delete;

    class data_t;
    std::unique_ptr<data_t> m_data;
};

template<> struct database::sysObj_t<sysrowsets>      { static const sysObj id = sysObj::sysrowsets; };
template<> struct database::sysObj_t<sysschobjs>      { static const sysObj id = sysObj::sysschobjs; };
template<> struct database::sysObj_t<syscolpars>      { static const sysObj id = sysObj::syscolpars; };
template<> struct database::sysObj_t<sysscalartypes>  { static const sysObj id = sysObj::sysscalartypes; };
template<> struct database::sysObj_t<sysidxstats>     { static const sysObj id = sysObj::sysidxstats; };
template<> struct database::sysObj_t<sysiscols>       { static const sysObj id = sysObj::sysiscols; };
template<> struct database::sysObj_t<sysobjvalues>    { static const sysObj id = sysObj::sysobjvalues; };

} // db
} // sdl

#include "database.inl"

#endif // __SDL_SYSTEM_DATABASE_H__
