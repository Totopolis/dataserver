// database.h
//
#ifndef __SDL_SYSTEM_DATABASE_H__
#define __SDL_SYSTEM_DATABASE_H__

#pragma once

#include "datapage.h"
#include "usertable.h"
#include "page_iterator.h"
#include "page_info.h"
#include "map_enum.h"
#include <map>

namespace sdl { namespace db {

class datatable;
class database: noncopyable
{
    enum class sysObj {
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
public:
    template<class T> using page_ptr = std::shared_ptr<T>;

    using shared_usertable = std::shared_ptr<usertable>;
    using vector_shared_usertable = std::vector<shared_usertable>;
    
    using shared_datatable = std::shared_ptr<datatable>; 
    using vector_shared_datatable = std::vector<shared_datatable>; 

    using unique_datatable = std::unique_ptr<datatable>;
    using shared_datapage = std::shared_ptr<datapage>; 
    using shared_iam_page = std::shared_ptr<iam_page>;
public:   
    void load_page(page_ptr<sysallocunits> &);
    void load_page(page_ptr<sysschobjs> &);
    void load_page(page_ptr<syscolpars> &);
    void load_page(page_ptr<sysidxstats> &);
    void load_page(page_ptr<sysscalartypes> &);
    void load_page(page_ptr<sysobjvalues> &);
    void load_page(page_ptr<sysiscols> &);
    void load_page(page_ptr<pfs_page> &);

    void load_next(page_ptr<sysallocunits> &);
    void load_next(page_ptr<sysschobjs> &);
    void load_next(page_ptr<syscolpars> &);
    void load_next(page_ptr<sysidxstats> &);
    void load_next(page_ptr<sysscalartypes> &);
    void load_next(page_ptr<sysobjvalues> &);
    void load_next(page_ptr<sysiscols> &);
    void load_next(page_ptr<pfs_page> &);

    void load_prev(page_ptr<sysallocunits> &);
    void load_prev(page_ptr<sysschobjs> &);
    void load_prev(page_ptr<syscolpars> &);
    void load_prev(page_ptr<sysidxstats> &);
    void load_prev(page_ptr<sysscalartypes> &);
    void load_prev(page_ptr<sysobjvalues> &);
    void load_prev(page_ptr<sysiscols> &);
    void load_prev(page_ptr<pfs_page> &);

    void load_next(shared_datapage &);
    void load_prev(shared_datapage &);

    void load_next(shared_iam_page &);
    void load_prev(shared_iam_page &);

    template<typename T> void load_page(T&) = delete;
    template<typename T> void load_next(T&) = delete;
    template<typename T> void load_prev(T&) = delete;

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
        using iterator = page_iterator<database, shared_iam_page>;// , iam_access > ;
        explicit iam_access(database * p, sysallocunits_row const * a)
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
    page_head const * load_sys_obj(sysallocunits const *, sysObj);

    template<class T, sysObj id> 
    page_ptr<T> get_sys_obj(sysallocunits const *);
    
    template<class T, sysObj id> 
    page_ptr<T> get_sys_obj();

    template<class T>
    void load_next_t(page_ptr<T> &);

    template<class T>
    void load_prev_t(page_ptr<T> &);

    template<class T, class fun_type> static
    typename T::const_pointer 
    find_row_if(page_access<T> & obj, fun_type fun) {
        for (auto & p : obj) {
            if (auto found = p->find_if(fun)) {
                return found;
            }
        }
        return nullptr;
    }
    template<class T, class fun_type> static
    void for_row(page_access<T> & obj, fun_type fun) {
        for (auto & p : obj) {
            p->for_row(fun);
        }
    }
    template<class fun_type>
    unique_datatable find_table_if(fun_type);
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
    
    void const * start_address() const; // diagnostic only
    void const * memory_offset(void const *) const; // diagnostic only

    pageType get_pageType(pageFileID const &);
    pageFileID nextPageID(pageFileID const &);
    pageFileID prevPageID(pageFileID const &);

    page_ptr<bootpage> get_bootpage();
    page_ptr<pfs_page> get_pfs_page();
    page_ptr<fileheader> get_fileheader();

    page_ptr<datapage> get_datapage(pageIndex);

    page_ptr<sysallocunits> get_sysallocunits();
    page_ptr<sysschobjs> get_sysschobjs();
    page_ptr<syscolpars> get_syscolpars();
    page_ptr<sysidxstats> get_sysidxstats();
    page_ptr<sysscalartypes> get_sysscalartypes();
    page_ptr<sysobjvalues> get_sysobjvalues();
    page_ptr<sysiscols> get_sysiscols();   

    page_access<sysallocunits> _sysallocunits{this};
    page_access<sysschobjs> _sysschobjs{this};
    page_access<syscolpars> _syscolpars{this};
    page_access<sysidxstats> _sysidxstats{this};
    page_access<sysscalartypes> _sysscalartypes{this};
    page_access<sysobjvalues> _sysobjvalues{this};
    page_access<sysiscols> _sysiscols{ this };
    page_access<pfs_page> _pfs_page{ this };

    usertable_access _usertables{this};
    datatable_access _datatables{this};

    unique_datatable find_table_name(const std::string & name);

    using vector_sysallocunits_row = std::vector<sysallocunits_row const *>;
    using vector_page_head = std::vector<page_head const *>;

    vector_sysallocunits_row const & find_sysalloc(schobj_id, dataType::type);
    vector_page_head const & find_datapage(schobj_id, dataType::type, pageType::type);
    
    shared_iam_page load_iam_page(pageFileID const &);

    iam_access pgfirstiam(sysallocunits_row const * it) { 
        return iam_access(this, it); 
    }
    bool is_allocated(pageFileID const &);

    auto get_access(impl::identity<sysallocunits>)  -> decltype((_sysallocunits))   { return _sysallocunits; }
    auto get_access(impl::identity<sysschobjs>)     -> decltype((_sysschobjs))      { return _sysschobjs; }
    auto get_access(impl::identity<syscolpars>)     -> decltype((_syscolpars))      { return _syscolpars; }
    auto get_access(impl::identity<sysidxstats>)    -> decltype((_sysidxstats))     { return _sysidxstats; }
    auto get_access(impl::identity<sysscalartypes>) -> decltype((_sysscalartypes))  { return _sysscalartypes; }
    auto get_access(impl::identity<sysobjvalues>)   -> decltype((_sysobjvalues))    { return _sysobjvalues; }
    auto get_access(impl::identity<sysiscols>)      -> decltype((_sysiscols))       { return _sysiscols; }

    template<class T> 
    auto get_access_t() -> decltype(get_access(impl::identity<T>())) {
        return this->get_access(impl::identity<T>());
    }
private:
    template<class fun_type>
    void for_sysschobjs(fun_type fun) {
        for (auto & p : _sysschobjs) {
            p->for_row(fun);
        }
    }
    template<class fun_type>
    void for_USER_TABLE(fun_type fun) {
        for_sysschobjs([&fun](sysschobjs::const_pointer row){
            if (row->is_USER_TABLE_id()) {
                fun(row);
            }
        });
    }
    vector_shared_usertable const & get_usertables();
    vector_shared_datatable const & get_datatable();

    page_head const * load_page_head(sysPage);
    std::vector<page_head const *> load_page_list(page_head const *);
private:
    class data_t;
    std::unique_ptr<data_t> m_data;
};

template<class T> inline
auto get_access(database & db) -> decltype(db.get_access_t<T>())
{
    return db.get_access_t<T>();
}

class datatable : noncopyable
{
    using shared_usertable = database::shared_usertable;
    using shared_datapage = database::shared_datapage;
    using shared_iam_page = database::shared_iam_page;
private:
    database * const db;
    shared_usertable const schema;
private:
    class sysalloc_access {
        using vector_data = database::vector_sysallocunits_row;
        datatable * const table;
        dataType::type const data_type;
    public:
        using iterator = vector_data::const_iterator;
        sysalloc_access(datatable * p, dataType::type t)
            : table(p), data_type(t)
        {
            SDL_ASSERT(table);
            SDL_ASSERT(data_type != dataType::type::null);
        }
        iterator begin() {
            return find_sysalloc().begin();
        }
        iterator end() {
            return find_sysalloc().end();
        }
    private:
        vector_data const & find_sysalloc() const {
           return table->db->find_sysalloc(table->get_id(), data_type);
        }
    };
    class datapage_access {
        using vector_data = database::vector_page_head;
        datatable * const table;
        dataType::type const data_type;
        pageType::type const page_type;
    public:
        using iterator = vector_data::const_iterator;
        datapage_access(datatable * p, dataType::type t1, pageType::type t2)
            : table(p), data_type(t1), page_type(t2)
        {
            SDL_ASSERT(table);
            SDL_ASSERT(data_type != dataType::type::null);
            SDL_ASSERT(page_type != pageType::type::null);
        }
        iterator begin() {
            return find_datapage().begin();
        }
        iterator end() {
            return find_datapage().end();
        }
    private:
        vector_data const & find_datapage() const {
           return table->db->find_datapage(table->get_id(), data_type, page_type);
        }
    };
    class datarow_access_base {
    protected:
        datatable * const table;
        datapage_access _datapage;
        using page_slot = std::pair<datapage_access::iterator, size_t>;        
        void load_next_row(page_slot &);
        void load_prev_row(page_slot &);
        bool is_empty(page_slot const &);
        bool is_begin(page_slot const &);
    public:
        datarow_access_base(datatable * p, dataType::type t1, pageType::type t2)
            : table(p), _datapage(p, t1, t2)
        {
            SDL_ASSERT(table);
        }
        bool is_end(page_slot const &);
    };
    class datarow_access: datarow_access_base {
        enum { assert_empty_slot = 0 };
    public:
        using iterator = page_iterator<datarow_access, page_slot>;
        datarow_access(datatable * p, dataType::type t1, pageType::type t2)
            : datarow_access_base(p, t1, t2)
        {}
        iterator begin();
        iterator end();
    private:
        friend iterator;
        void load_next(page_slot &);
        void load_prev(page_slot &);
        static bool is_same(page_slot const &, page_slot const &);
        row_head const * dereference(page_slot const &);
    };
public:
    datatable(database * p, shared_usertable const & t);
    ~datatable();

    const std::string & name() const {
        return schema->name();
    }
    schobj_id get_id() const {
        return schema->get_id();
    }
    const usertable & ut() const {
        return *schema.get();
    }
    sysalloc_access _sysalloc(dataType::type t1) {
        return sysalloc_access(this, t1);
    }
    datapage_access _datapage(dataType::type t1, pageType::type t2) {
        return datapage_access(this, t1, t2);
    }
    datarow_access _datarow(dataType::type t1, pageType::type t2) {
        return datarow_access(this, t1, t2);
    }
    template<class T, class fun_type> static
    void for_datarow(T && data, fun_type fun) {
        A_STATIC_ASSERT_TYPE(datarow_access, std::remove_reference<T>::type);
        for (row_head const * row : data) {
            if (row) {
                fun(row);
            }
        }
    }
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_H__
