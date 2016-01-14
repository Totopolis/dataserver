// database.h
//
#ifndef __SDL_SYSTEM_DATABASE_H__
#define __SDL_SYSTEM_DATABASE_H__

#pragma once

#include "datapage.h"

namespace sdl { namespace db {

class tablecolumn : noncopyable
{
    syscolpars_row const * const colpar; // id, colid, utype, length
    sysscalartypes_row const * const scalar; // id, name
public:
    struct data_type {
        std::string name;
        scalartype type = scalartype::t_none;
        int16 length = 0; //  -1 if this is a varchar(max) / text / image data type with no practical maximum length
        data_type(const std::string & s): name(s) {}
        bool is_varlength() const {
            return (this->length == -1);
        }
    };
public:
    tablecolumn(
        syscolpars_row const *,
        sysscalartypes_row const *,
        const std::string & _name);

    data_type const & data() const { 
        return m_data;
    }
private:
    data_type m_data;
};

class tableschema : noncopyable
{
    sysschobjs_row const * const schobj; // id, name
public:
    typedef std::vector<std::unique_ptr<tablecolumn>> cols_type;
public:
    explicit tableschema(sysschobjs_row const *);
    int32 get_id() const {
        return schobj->data.id;
    }
    cols_type const & cols() const {
        return m_cols;
    }
    void push_back(std::unique_ptr<tablecolumn>);
private:
    cols_type m_cols;
};

class usertable : noncopyable
{
public:
    usertable(sysschobjs_row const *, const std::string & _name);

    int32 get_id() const { 
        return m_sch.get_id();
    }
    const std::string & name() const {
        return m_name;
    }
    tableschema const & sch() const {
        return m_sch;
    }
    void push_back(std::unique_ptr<tablecolumn>);

    static std::string type_sch(usertable const &);
private:
    tableschema m_sch;
    const std::string m_name;
};

template<class T, class _value_type>
class page_iterator : public std::iterator<
        std::bidirectional_iterator_tag,
        _value_type>
{
public:
    using value_type = _value_type;
private:
    T * parent;
    value_type current; // std::unique_ptr or std::shared_ptr

    friend T;
    page_iterator(T * p, value_type && v): parent(p), current(std::move(v)) {
        SDL_ASSERT(parent);
    }
    explicit page_iterator(T * p): parent(p) {
        SDL_ASSERT(parent);
    }
public:
    page_iterator() : parent(nullptr), current() {}

    page_iterator & operator++() { // preincrement
        SDL_ASSERT(parent && current.get());
        parent->load_next(current);
        return (*this);
    }
    page_iterator operator++(int) { // postincrement
        auto temp = *this;
        ++(*this);
        SDL_ASSERT(temp != *this);
        return temp;
    }
    page_iterator & operator--() { // predecrement
        SDL_ASSERT(parent && current.get());
        parent->load_prev(current);
        return (*this);
    }
    page_iterator operator--(int) { // postdecrement
        auto temp = *this;
        --(*this);
        SDL_ASSERT(temp != *this);
        return temp;
    }
    bool operator==(const page_iterator& it) const {
        SDL_ASSERT(!parent || !it.parent || (parent == it.parent));
        return 
            (parent == it.parent) &&
            (current == it.current);
    }
    bool operator!=(const page_iterator& it) const {
        return !(*this == it);
    }
    value_type const & operator*() const {
        SDL_ASSERT(parent && current.get());
        return current;
    }
    value_type const * operator->() const {
        return &(**this);
    }
};

class database : noncopyable
{
    enum class sysObj {
        sysallocunits = 7,
        sysschobjs = 34,
        syscolpars = 41,
        sysscalartypes = 50,
        sysidxstats = 54,
        sysiscols = 55,
        sysobjvalues = 60,
    };
    enum class sysPage {
        file_header = 0,
        boot_page = 9,
    };
public:
    template<class T> using page_ptr = std::unique_ptr<T>; //std::shared_ptr<T>;
    template<class T> using vector_page_ptr = std::vector<page_ptr<T>>;

    using shared_usertable = std::shared_ptr<usertable>;
    using vector_usertable = std::vector<shared_usertable>;
public:   
    void load_page(page_ptr<sysallocunits> &);
    void load_page(page_ptr<sysschobjs> &);
    void load_page(page_ptr<syscolpars> &);
    void load_page(page_ptr<sysidxstats> &);
    void load_page(page_ptr<sysscalartypes> &);
    void load_page(page_ptr<sysobjvalues> &);
    void load_page(page_ptr<sysiscols> &);

    void load_next(page_ptr<sysallocunits> &);
    void load_next(page_ptr<sysschobjs> &);
    void load_next(page_ptr<syscolpars> &);
    void load_next(page_ptr<sysidxstats> &);
    void load_next(page_ptr<sysscalartypes> &);
    void load_next(page_ptr<sysobjvalues> &);
    void load_next(page_ptr<sysiscols> &);

    void load_prev(page_ptr<sysallocunits> &);
    void load_prev(page_ptr<sysschobjs> &);
    void load_prev(page_ptr<syscolpars> &);
    void load_prev(page_ptr<sysidxstats> &);
    void load_prev(page_ptr<sysscalartypes> &);
    void load_prev(page_ptr<sysobjvalues> &);
    void load_prev(page_ptr<sysiscols> &);

    void load_page(shared_usertable &);
    void load_next(shared_usertable &);
    void load_prev(shared_usertable &);

    template<typename T> void load_page(T&) = delete;
    template<typename T> void load_next(T&) = delete;
    template<typename T> void load_prev(T&) = delete;
private: 
    template<class pointer_type>
    class page_access_t : noncopyable {
        database * const db;
    public:
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
private:
    page_head const * load_sys_obj(sysallocunits const *, sysObj);

    template<class T, sysObj id> 
    page_ptr<T> get_sys_obj(sysallocunits const *);
    
    template<class T, sysObj id> 
    page_ptr<T> get_sys_obj();

    template<class T> 
    vector_page_ptr<T> get_sys_list(page_ptr<T> &&);

    template<class T, sysObj id> 
    vector_page_ptr<T> get_sys_list();

    template<class T>
    void load_next_t(page_ptr<T> &);

    template<class T>
    void load_prev_t(page_ptr<T> &);
public:
    explicit database(const std::string & fname);
    ~database();

    const std::string & filename() const;

    bool is_open() const;

    size_t page_count() const;

    page_head const * load_page_head(pageIndex);
    page_head const * load_page_head(pageFileID const &);

    void const * start_address() const; // diagnostic only
    void const * memory_offset(void const *) const; // diagnostic only

    page_ptr<bootpage> get_bootpage();
    page_ptr<fileheader> get_fileheader();
    page_ptr<datapage> get_datapage(pageIndex);

    page_ptr<sysallocunits> get_sysallocunits();
    page_ptr<sysschobjs> get_sysschobjs();
    page_ptr<syscolpars> get_syscolpars();
    page_ptr<sysidxstats> get_sysidxstats();
    page_ptr<sysscalartypes> get_sysscalartypes();
    page_ptr<sysobjvalues> get_sysobjvalues();
    page_ptr<sysiscols> get_sysiscols();   

//private: will be replaced by iterators
    vector_page_ptr<sysallocunits> get_sysallocunits_list();
    vector_page_ptr<sysschobjs> get_sysschobjs_list();
    vector_page_ptr<syscolpars> get_syscolpars_list();
    vector_page_ptr<sysscalartypes> get_sysscalartypes_list();
    vector_page_ptr<sysidxstats> get_sysidxstats_list();
    vector_page_ptr<sysobjvalues> get_sysobjvalues_list();
    vector_page_ptr<sysiscols> get_sysiscols_list(); 
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
public:
    vector_usertable get_usertables();
    page_access<sysallocunits> _sysallocunits{this};
    page_access<sysschobjs> _sysschobjs{this};
    page_access<syscolpars> _syscolpars{this};
    page_access<sysidxstats> _sysidxstats{this};
    page_access<sysscalartypes> _sysscalartypes{this};
    page_access<sysobjvalues> _sysobjvalues{this};
    page_access<sysiscols> _sysiscols{this};
    page_access_t<shared_usertable> _usertables{this};
private:
    page_head const * load_page_head(sysPage);
    page_head const * load_next_head(page_head const *);
    page_head const * load_prev_head(page_head const *);
    std::vector<page_head const *> load_page_list(page_head const *);
private:
    class data_t;
    std::unique_ptr<data_t> m_data;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_H__
