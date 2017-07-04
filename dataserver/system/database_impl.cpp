// database_impl.cpp
//
#include "dataserver/system/database.h"
#include "dataserver/system/database_impl.h"
#include "dataserver/filesys/file_map.h"

namespace sdl { namespace db {

#if SDL_TEST_PAGE_POOL

PagePool::PagePool(const std::string & fname)
    : m_file(fname, std::ifstream::in | std::ifstream::binary)
{
    throw_error_if_not<this_error>(m_file.is_open(), "file not found");
    m_file.seekg(0, std::ios_base::end);
    const auto filesize = m_file.tellg();
    m_alloc.reset(new mmu::vm_malloc(filesize));
}

page_head const *
PagePool::load_page(pageIndex const i)
{
    const size_t pageId = i.value(); // uint32 => size_t
    if (pageId < page_count()) {
        lock_guard lock(m_mutex);
        const bool loaded = m_alloc->is_commit(pageId);
        if (void * const ptr = m_alloc->alloc_page(pageId)) {
            if (loaded) {
                page_head const * const head = reinterpret_cast<page_head const *>(ptr);
                SDL_ASSERT(head->valid_checksum() || !head->data.tornBits);
                SDL_ASSERT(head->data.pageId.pageId == pageId);
                return head;
            }
            SDL_ASSERT(m_alloc->is_commit(pageId));
            const size_t offset = pageId * page_head::page_size;
            m_file.seekg(offset, std::ios_base::beg);
            m_file.read(reinterpret_cast<char*>(ptr), page_head::page_size);
            page_head const * const head = reinterpret_cast<page_head const *>(ptr);
            SDL_ASSERT(head->valid_checksum() || !head->data.tornBits);
            SDL_ASSERT(head->data.pageId.pageId == pageId);
            return head;
        }
    }
    SDL_TRACE("page not found: ", pageId);
    throw_error<this_error>("page not found");
    return nullptr;
}

#endif

} // db
} // sdl