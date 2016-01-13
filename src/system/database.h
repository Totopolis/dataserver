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
    template<class T> using page_ptr = std::shared_ptr<T>;
    template<class T> using vector_page_ptr = std::vector<page_ptr<T>>;
private:    
#if 0
    template<class T>
    class page_iterator : public std::iterator<
            std::bidirectional_iterator_tag,
            typename T::value_type>
    {
    public:
        using value_type = typename T::value_type;
    private:
        T * parent;
        value_type current{};

        friend T;
        page_iterator(T * p, value_type && v)
            : parent(p)
            , current(v)
        {
            SDL_ASSERT(parent);
        }
    public:
        page_iterator() : parent(nullptr){}

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

    //template<class page_type>
    class page_access : noncopyable {
        using page_type = sysallocunits;
        database & parent;
    public:
        using value_type = page_ptr<page_type>;
        explicit page_access(database & p): parent(p) {}
    public:
        using iterator = page_iterator<page_access>;
        iterator begin() {
            return iterator(this, load_first());
        }
        iterator end() {
            return iterator(this, value_type{});
        }
        value_type load_first();
        void load_next(value_type &);
        void load_prev(value_type &);
    };
    page_access m_sysallocunits;
#endif
private:
    page_head const * load_sys_obj(sysallocunits const *, sysObj);

    template<class T, sysObj id> 
    page_ptr<T> get_sys_obj(sysallocunits const * p);
    
    template<class T, sysObj id> 
    page_ptr<T> get_sys_obj();

    template<class T> 
    vector_page_ptr<T> get_sys_list(page_ptr<T> &&);

    template<class T, sysObj id> 
    vector_page_ptr<T> get_sys_list();
public:
    explicit database(const std::string & fname);
    ~database();

    const std::string & filename() const;

    bool is_open() const;

    size_t page_count() const;

    page_head const * load_page(pageIndex);
    page_head const * load_page(pageFileID const &);

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

    vector_page_ptr<sysallocunits> get_sysallocunits_list();
    vector_page_ptr<sysschobjs> get_sysschobjs_list();
    vector_page_ptr<syscolpars> get_syscolpars_list();
    vector_page_ptr<sysscalartypes> get_sysscalartypes_list();
    vector_page_ptr<sysidxstats> get_sysidxstats_list();
    vector_page_ptr<sysobjvalues> get_sysobjvalues_list();
    vector_page_ptr<sysiscols> get_sysiscols_list(); 

    template<class fun_type>
    void for_sysschobjs(fun_type fun) {
        for (auto & p : get_sysschobjs_list()) {
            p->for_row(fun);
        }
    }
    template<class fun_type>
    void for_USER_TABLE(fun_type fun) {
        for_sysschobjs([&fun](sysschobjs_row const * const row){
            if (row->is_USER_TABLE_id()) {
                fun(row);
            }
        });
    }
    using vector_usertable = vector_page_ptr<usertable>;
    vector_usertable get_usertables();
private:
    page_head const * load_page(sysPage);
    page_head const * load_next(page_head const *);
    page_head const * load_prev(page_head const *);
    std::vector<page_head const *> load_page_list(page_head const *);
private:
    class data_t;
    std::unique_ptr<data_t> m_data;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_DATABASE_H__
