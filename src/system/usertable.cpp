// usertable.cpp
//
#include "common/common.h"
#include "usertable.h"

#include <algorithm>
#include <sstream>

namespace sdl { namespace db {

//----------------------------------------------------------------------------

tablecolumn::tablecolumn(syscolpars_row const * _colpar,
                         sysscalartypes_row const * _scalar,
                         const std::string & _name)
    : colpar(_colpar)
    , scalar(_scalar)
    , m_data(_name)
{
    SDL_ASSERT(colpar);
    SDL_ASSERT(scalar);    
    SDL_ASSERT(colpar->data.utype == scalar->data.id);

    A_STATIC_SAME_TYPE(colpar->data.utype, scalar->data.id);
    A_STATIC_SAME_TYPE(m_data.length, colpar->data.length);

    m_data.length = colpar->data.length;
    m_data.type = scalar->id_scalartype();
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

void usertable::push_back(std::unique_ptr<tablecolumn> p)
{
    SDL_ASSERT(p);
    m_cols.push_back(std::move(p));
}

std::string usertable::type_schema(usertable const & ut)
{
    auto & cols = ut.cols();
    std::stringstream ss;
    ss  << "name = " << ut.name()
        << "\nid = " << ut.get_id()._32
        << std::uppercase << std::hex 
        << " (" << ut.get_id()._32 << ")"
        << std::dec
        << "\nColumns(" << cols.size() << ")"
        << "\n";
    for (auto & p : cols) {
        auto const & d = p->data();
        ss << d.name << " : "
            << scalartype_info::type(d.type)
            << " (";
        if (d.is_varlength())
            ss << "var";
        else 
            ss << d.length;
        ss << ")\n";
    }
    return ss.str();
}

} // db
} // sdl

