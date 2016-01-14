// usertable.h
//
#ifndef __SDL_SYSTEM_USERTABLE_H__
#define __SDL_SYSTEM_USERTABLE_H__

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
    bool empty() const {
        return m_cols.empty();
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
    bool empty() const {
        return m_sch.empty();
    }
    void push_back(std::unique_ptr<tablecolumn>);

    static std::string type_sch(usertable const &);
private:
    tableschema m_sch;
    const std::string m_name;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_USERTABLE_H__
