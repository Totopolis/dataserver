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

usertable::usertable(sysschobjs_row const * p, const std::string & _name)
    : schobj(p)
    , m_name(_name)
{
    SDL_ASSERT(!m_name.empty());
    SDL_ASSERT(schobj);
    SDL_ASSERT(schobj->is_USER_TABLE_id());
    A_STATIC_SAME_TYPE(sysschobjs_row().data.id, get_id());
}

std::string usertable::type_schema() const
{
    usertable const & ut = *this;
    auto & cols = ut.cols();
    std::stringstream ss;
    ss  << "name = " << ut.name()
        << "\nid = " << ut.get_id()._32
        << std::uppercase << std::hex 
        << " (" << ut.get_id()._32 << ")"
        << std::dec
        << "\nColumns(" << cols.size() << ")"
        << "\n";
    for (auto & d : cols) {
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

