// page_bpool.cpp
//
#include "dataserver/bpool/page_bpool.h"

namespace sdl { namespace db { namespace bpool {

pool_info_t::pool_info_t(const size_t s)
    : filesize(s)
    , page_count(s / page_size)
    , block_count((s + block_size - 1) / block_size)
{
    SDL_ASSERT(filesize > block_size);
    SDL_ASSERT(!(filesize % page_size));
    static_assert(is_power_two(block_page_num), "");
    const size_t n = page_count % block_page_num;
    last_block = block_count - 1;
    last_block_page_count = n ? n : block_page_num;
    last_block_size = page_size * last_block_page_count;
    SDL_ASSERT((last_block_size >= page_size) && (last_block_size <= block_size));
}

//---------------------------------------------------

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

page_bpool::page_bpool(const std::string & fname,
                       const size_t min_size,
                       const size_t max_size)
    : base_page_bpool(fname)
    , min_pool_size(min_size ? a_min(min_size, m_file.filesize()) : min_size)
    , max_pool_size(max_size ? a_min(max_size, m_file.filesize()) : max_size)
    , info(m_file.filesize())
{
    SDL_ASSERT(min_size <= max_size);
    SDL_ASSERT(min_pool_size <= max_pool_size);
}

page_bpool::page_bpool(const std::string & fname)
    : page_bpool(fname, 0, 0)
{
}

page_bpool::~page_bpool()
{
}

bool page_bpool::is_open() const
{
    return false;
}

void const * page_bpool::start_address() const
{
    return nullptr;
}

size_t page_bpool::page_count() const
{
    return info.page_count;
}

page_head const *
page_bpool::lock_page(pageIndex pageId)
{
    return nullptr;
}

bool page_bpool::unlock_page(pageIndex)
{
    return false;
}

#if SDL_DEBUG
bool page_bpool::assert_page(pageFileID id)
{
    if (id) {
        return lock_page(id.pageId) != nullptr;
    }
    return true;
}
#endif

}}} // sdl
