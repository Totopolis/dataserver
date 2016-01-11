// database.h
//
#ifndef __SDL_SYSTEM_DATABASE_H__
#define __SDL_SYSTEM_DATABASE_H__

#pragma once

#include "datapage.h"

namespace sdl { namespace db {

namespace usr { // user table

enum class scalartype
{
    t_none              = 0,
    t_image	            = 34,
    t_text	            = 35,	
    t_uniqueidentifier	= 36,	
    t_date	            = 40,	
    t_time	            = 41,	
    t_datetime2	        = 42,	
    t_datetimeoffset	= 43,	
    t_tinyint	        = 48,	
    t_smallint        	= 52,	
    t_int             	= 56,	
    t_smalldatetime   	= 58,	
    t_real            	= 59,	
    t_money           	= 60,	
    t_datetime	        = 61,	
    t_float           	= 62,	
    t_sql_variant     	= 98,	
    t_ntext	            = 99,	
    t_bit	            = 104,	
    t_decimal         	= 106,	
    t_numeric         	= 108,	
    t_smallmoney      	= 122,	
    t_bigint          	= 127,	
    t_hierarchyid     	= 128,	
    t_geometry        	= 129,	
    t_geography	        = 130,	
    t_varbinary	        = 165,	
    t_varchar	        = 167,	
    t_binary	        = 173,	
    t_char            	= 175,	
    t_timestamp       	= 189,	
    t_nvarchar	        = 231,	
    t_nchar           	= 239,	
    t_xml	            = 241,	
    t_sysname	        = 256,
};

class tablecolumn : noncopyable
{
    syscolpars_row const * const colpar; // id, colid, utype, length
    sysscalartypes_row const * const scalar; // id, name
public:
    tablecolumn(
        syscolpars_row const *,
        sysscalartypes_row const *,
        const std::string & _name);
private:
    struct data_type {
        const std::string name;
        scalartype type;
        int16 length; //  -1 if this is a varchar(max) / text / image data type with no practical maximum length
        data_type(const std::string & s)
            : name(s)
            , type(scalartype::t_none)
            , length(0)
        {}
    };
    data_type data;
};

class tableschema : noncopyable
{
    sysschobjs_row const * const schobj; // id, name
public:
    explicit tableschema(sysschobjs_row const *);
private:
    typedef std::vector<std::unique_ptr<tablecolumn>> col_type;
    col_type col;
};

class usertable : noncopyable
{
public:
    usertable(sysschobjs_row const *, const std::string & _name);
private:
    tableschema scheme;
    const std::string name;
};

} // usr

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

    //FIXME: get user tables scheme

public:
    void const * start_address() const; // diagnostic only
    void const * memory_offset(void const * p) const; // diagnostic only
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
