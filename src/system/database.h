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
private:
    page_head const * load_sys_obj(sysallocunits const *, sysObj);

    template<class T, sysObj id> 
    std::unique_ptr<T> get_sys_obj(sysallocunits const * p);
    
    template<class T, sysObj id> 
    std::unique_ptr<T> get_sys_obj();

    template<class T> 
    std::vector<std::unique_ptr<T>> get_sys_list(std::unique_ptr<T>);

    template<class T, sysObj id> 
    std::vector<std::unique_ptr<T>> get_sys_list();
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

    std::unique_ptr<bootpage> get_bootpage();
    std::unique_ptr<fileheader> get_fileheader();
    std::unique_ptr<datapage> get_datapage(pageIndex);

    std::unique_ptr<sysallocunits> get_sysallocunits();
    std::unique_ptr<sysschobjs> get_sysschobjs();
    std::unique_ptr<syscolpars> get_syscolpars();
    std::unique_ptr<sysidxstats> get_sysidxstats();
    std::unique_ptr<sysscalartypes> get_sysscalartypes();
    std::unique_ptr<sysobjvalues> get_sysobjvalues();
    std::unique_ptr<sysiscols> get_sysiscols();   

    std::vector<std::unique_ptr<sysallocunits>> get_sysallocunits_list();
    std::vector<std::unique_ptr<sysschobjs>> get_sysschobjs_list();
    std::vector<std::unique_ptr<syscolpars>> get_syscolpars_list();
    std::vector<std::unique_ptr<sysscalartypes>> get_sysscalartypes_list();
    std::vector<std::unique_ptr<sysidxstats>> get_sysidxstats_list();
    std::vector<std::unique_ptr<sysobjvalues>> get_sysobjvalues_list();
    std::vector<std::unique_ptr<sysiscols>> get_sysiscols_list(); 

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
    typedef std::vector<std::unique_ptr<usertable>> vector_usertable;
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
