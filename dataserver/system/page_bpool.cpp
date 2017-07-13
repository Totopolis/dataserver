// page_bpool.cpp
//
#include "dataserver/system/page_bpool.h"

namespace sdl { namespace db { namespace bpool {

base_page_bpool::base_page_bpool(const std::string & fname)
    : m_file(fname) 
{
    throw_error_if_not_t<base_page_bpool>(m_file.is_open() && m_file.filesize(), "bad file");
    throw_error_if_not_t<base_page_bpool>(valid_filesize(m_file.filesize()), "bad filesize");
}

base_page_bpool::~base_page_bpool()
{
}

bool base_page_bpool::valid_filesize(const size_t filesize) {
    using T = pool_limits;
    if (filesize > T::block_size) {
        if (!(filesize % T::page_size)) {
            return true;
        }
    }
    SDL_ASSERT(!"valid_filesize");
    return false;
}

//------------------------------------------------------

page_bpool::page_bpool(const std::string & fname, const size_t s)
    : base_page_bpool(fname)
    , max_pool_size(s ? a_min(s, m_file.filesize()) : s)
{
    SDL_ASSERT(max_pool_size <= m_file.filesize());
}

page_bpool::~page_bpool()
{
}

}}} // sdl
