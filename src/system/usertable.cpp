// usertable.cpp
//
#include "common/common.h"
#include "usertable.h"

#include <algorithm>
#include <sstream>

namespace sdl { namespace db {

tablecolumn::tablecolumn(syscolpars_row const * colpar,
                         sysscalartypes_row const * scalar,
                         const std::string & _name)
    : name(_name)
    , type(scalar->id_scalartype())
    , length(colpar->data.length)
{
    SDL_ASSERT(colpar);
    SDL_ASSERT(scalar);    
    SDL_ASSERT(colpar->data.utype == scalar->data.id);
    A_STATIC_SAME_TYPE(colpar->data.utype, scalar->data.id);
}

//----------------------------------------------------------------------------

usertable::usertable(sysschobjs_row const * p,
                     std::string && n, 
                     columns && c)
    : schobj(p)
    , id(schobj->data.id)
    , name(std::move(n))
    , cols(std::move(c))
{
    SDL_ASSERT(schobj);
    SDL_ASSERT(schobj->is_USER_TABLE_id());

    SDL_ASSERT(!name.empty());
    SDL_ASSERT(!cols.empty());
    SDL_ASSERT(this->id._32);
}

std::string usertable::type_schema() const
{
    usertable const & ut = *this;
    std::stringstream ss;
    ss  << "name = " << ut.name
        << "\nid = " << ut.id._32
        << std::uppercase << std::hex 
        << " (" << ut.id._32 << ")"
        << std::dec
        << "\nColumns(" << ut.cols.size() << ")"
        << "\n";
    for (auto & d : ut.cols) {
        ss << d.name << " : "
            << scalartype_info::type(d.type)
            << " (";
        if (d.is_varlength())
            ss << "var";
        else 
            ss << d.length._16;
        ss << ")\n";
    }
    return ss.str();
}

} // db
} // sdl

