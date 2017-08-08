// database_impl.cpp
//
#include "dataserver/system/database.h"
#include "dataserver/system/database_impl.h"

namespace sdl { namespace db {

database_PageMapping::database_PageMapping(const std::string & fname, database_cfg const & cfg)
    : m_cfg(cfg)
{
    if (cfg.use_page_bpool) {
        reset_new(m_pool, fname, cfg);
    }
    else {
        reset_new(m_pmap, fname);
    }
}

database_PageMapping::~database_PageMapping()
{
}

bool database_PageMapping::is_open() const
{
    return m_pool ? m_pool->is_open() : m_pmap->is_open();
}

size_t database_PageMapping::page_count() const
{
    return m_pool ? m_pool->page_count() : m_pmap->page_count();
}

std::thread::id database_PageMapping::init_thread_id() const
{
    return m_pool ? m_pool->init_thread_id : m_pmap->init_thread_id;
}

} // db
} // sdl