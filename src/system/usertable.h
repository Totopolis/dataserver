// usertable.h
//
#ifndef __SDL_SYSTEM_USERTABLE_H__
#define __SDL_SYSTEM_USERTABLE_H__

#pragma once

#include "datapage.h"

namespace sdl { namespace db {

class tablecolumn 
{
public:
    const std::string name;
    const scalartype type;
    const scalarlen length; //  -1 if this is a varchar(max) / text / image data type with no practical maximum length

    bool is_varlength() const {
        return this->length.is_var();
    }
    tablecolumn(syscolpars_row const *,
                sysscalartypes_row const *, 
                const std::string & _name);
};

class usertable : noncopyable
{
    sysschobjs_row const * const schobj; // id, name
public:
    typedef std::vector<tablecolumn> cols_type;
public:
    usertable(sysschobjs_row const *, const std::string & _name);

    schobj_id get_id() const {
        return schobj->data.id;
    }
    const std::string & name() const {
        return m_name;
    }
    bool empty() const {
        return m_cols.empty();
    }
    size_t size() const {
        return m_cols.size();
    }
    cols_type const & cols() const {
        return m_cols;
    }
    tablecolumn const & operator[](size_t i) const {
        return m_cols[i];
    }
    template<typename... Ts>
    void emplace_back(Ts&&... params) {
        m_cols.emplace_back(std::forward<Ts>(params)...);
    }
    std::string type_schema() const;
private:
    const std::string m_name; 
    cols_type m_cols;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_USERTABLE_H__
