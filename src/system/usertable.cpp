// usertable.cpp
//
#include "common/common.h"
#include "usertable.h"

#include <algorithm>
#include <sstream>

namespace sdl { namespace db {

usertable::column::column(syscolpars_row const * colpar,
                          sysscalartypes_row const * scalar,
                          std::string && n)
    : name(std::move(n))
    , type(scalar->data.id)
    , length(colpar->data.length)
{
    SDL_ASSERT(colpar && scalar);    
    SDL_ASSERT(colpar->data.utype == scalar->data.id);
    SDL_ASSERT(this->type != scalartype::t_none);
}

bool usertable::column::is_fixed() const
{
    if (scalartype::is_fixed(this->type)) {
        return !length.is_var();
    }
    return false;
}

size_t usertable::column::fixed_size() const
{
    SDL_ASSERT(is_fixed());
    return length._16;
}

//----------------------------------------------------------------------------

usertable::usertable(sysschobjs_row const * p,
                     std::string && n, 
                     columns && c)
    : schobj(p)
    , name(std::move(n))
    , schema(std::move(c))
{
    SDL_ASSERT(schobj);
    SDL_ASSERT(schobj->is_USER_TABLE_id());

    SDL_ASSERT(!name.empty());
    SDL_ASSERT(!schema.empty());
    SDL_ASSERT(get_id()._32);
}

std::string usertable::type_schema() const
{
    usertable const & ut = *this;
    std::stringstream ss;
    ss  << "name = " << ut.name
        << "\nid = " << ut.get_id()._32
        << std::uppercase << std::hex 
        << " (" << ut.get_id()._32 << ")"
        << std::dec
        << "\nColumns(" << ut.schema.size() << ")"
        << "\n";
    for (auto & d : ut.schema) {
        ss << d.name << " : "
            << scalartype::get_name(d.type)
            << " (";
        if (d.length.is_var())
            ss << "var";
        else 
            ss << d.length._16;
        ss << ")";
        if (d.is_fixed()) {
            ss << " fixed";
        }
        ss << "\n";
    }
    return ss.str();
}

size_t usertable::count_var() const
{
    return count_if([](column_ref c){
        return !c.is_fixed();
    });
}

size_t usertable::count_fixed() const
{
    return count_if([](column_ref c){
        return c.is_fixed();
    });
}

} // db
} // sdl

