// page_pool.cpp
//
#include "dataserver/system/page_pool.h"

#if SDL_TEST_PAGE_POOL
namespace sdl { namespace db { namespace pp {
#if 1
#define SDL_PAGE_ASSERT(...)   SDL_ASSERT(__VA_ARGS__)
#else
#define SDL_PAGE_ASSERT(...)   ((void)0)
#endif

#if SDL_PAGE_POOL_STAT
thread_local PagePool::unique_page_stat
PagePool::thread_page_stat;
#endif

//https://msdn.microsoft.com/en-us/library/windows/desktop/aa364218(v=vs.85).aspx
// The amount of I/O performance improvement that file data caching offers depends on 
// the size of the file data block being read or written. 
// When large blocks of file data are read and written, 
// it is more likely that disk reads and writes will be necessary to finish the I/O operation. 
// I/O performance will be increasingly impaired as more of this kind of I/O operation occurs.
// In these situations, caching can be turned off. 
// This is done at the time the file is opened by passing FILE_FLAG_NO_BUFFERING as a value for the dwFlagsAndAttributes parameter of CreateFile.
// When caching is disabled, all read and write operations directly access the physical disk. 
// However, the file metadata may still be cached.
// To flush the metadata to disk, use the FlushFileBuffers function.

PagePool::PagePool(const std::string & fname)
    : m_file(fname, std::ifstream::in | std::ifstream::binary)
{
    SDL_TRACE_FUNCTION;
    static_assert(is_power_two(max_page), "");
    static_assert(is_power_two(slot_size), "");
    static_assert(power_of<slot_size>::value == 16, "");
    static_assert(gigabyte<1>::value / page_size == 131072, "");
    static_assert(gigabyte<5>::value / page_size == 655360, "");
    static_assert(gigabyte<1>::value / slot_size == 16384, "");
    static_assert(gigabyte<5>::value / slot_size == 81920, "");
    throw_error_if_not<this_error>(m_file.is_open(), "file not found");
    m_file.seekg(0, std::ios_base::end);
    m.filesize = m_file.tellg();
    m.page_count = m.filesize / page_size;
    m.slot_count = (m.filesize + slot_size - 1) / slot_size;
    SDL_PAGE_ASSERT((slot_page_num != 8) || (m.slot_count * slot_page_num == m.page_count));
    if (valid_filesize(m.filesize)) {
#if SDL_PAGE_POOL_SLOT
        m_slot_commit.resize(m.slot_count);
#else
        m_page_commit.resize(m.page_count);
#endif
        m_alloc.reset(new char[m.filesize]);
        throw_error_if_not<this_error>(is_open(), "bad alloc");
#if SDL_PAGE_POOL_LOAD_ALL
        load_all();
#endif
    }
    else {
        throw_error<this_error>("bad alloc size");
    }
}

#if SDL_DEBUG 
bool PagePool::check_page(page_head const * const head, const pageIndex pageId) {
    if (1) { // only for allocated page
        SDL_ASSERT(head->valid_checksum() || !head->data.tornBits);
        SDL_ASSERT(head->data.pageId.pageId == pageId.value());
    }
    return true;
}
#endif

bool PagePool::valid_filesize(const size_t filesize)
{
    if (filesize > slot_size) {
        SDL_PAGE_ASSERT(!(filesize % page_size));
        SDL_PAGE_ASSERT((slot_page_num != 8) || !(filesize % slot_size));
        return !(filesize % page_size);
    }
    return false;
}

void PagePool::load_all()
{
    SDL_TRACE(__FUNCTION__, " [", m.filesize, "] byte");
    SDL_UTILITY_SCOPE_TIMER_SEC(timer, "load_all seconds = ");
    m_file.seekg(0, std::ios_base::beg);
    m_file.read(m_alloc.get(), m.filesize);
    m_file.seekg(0, std::ios_base::beg);
#if SDL_PAGE_POOL_SLOT
    m_slot_commit.assign(m.slot_count, true);
#else
    m_page_commit.assign(m.page_count, true);
#endif
}

page_head const *
PagePool::load_page(pageIndex const index) {
    const size_t pageId = index.value(); // uint32 => size_t
    SDL_PAGE_ASSERT(pageId < m.page_count);
#if SDL_PAGE_POOL_STAT
    if (thread_page_stat) {
        thread_page_stat->load_page.insert((uint32)pageId);
        thread_page_stat->load_page_request++;
    }
#endif
    if (pageId < page_count()) {
        lock_guard lock(m_mutex);
        return load_page_nolock(index);
    }
    SDL_TRACE("page not found: ", pageId);
    throw_error<this_error>("page not found");
    return nullptr;
}

#if SDL_PAGE_POOL_SLOT
page_head const *
PagePool::load_page_nolock(pageIndex const index) {
    const size_t pageId = index.value(); // uint32 => size_t
    const size_t slotId = pageId / slot_page_num;
    SDL_PAGE_ASSERT(pageId < m.page_count);
    SDL_PAGE_ASSERT(slotId < m.slot_count);
#if SDL_PAGE_POOL_STAT
    if (thread_page_stat) {
        thread_page_stat->load_slot.insert((uint32)slotId);
    }
#endif
    char * const page_ptr = m_alloc.get() + pageId * page_size;
    if (!m_slot_commit[slotId]) { //FIXME: should use order access to file pages
        char * const slot_ptr = m_alloc.get() + slotId * slot_size;
        const size_t offset = slotId * slot_size;
        m_file.seekg(offset, std::ios_base::beg);
        if (slotId == m.last_slot()) {
            SDL_PAGE_ASSERT(slot_ptr + m.last_slot_size() == m_alloc.get() + m.filesize);
            m_file.read(slot_ptr, m.last_slot_size());
        }
        else {
            m_file.read(slot_ptr, slot_size);
        }
        m_slot_commit[slotId] = true;
    }
    page_head const * const head = reinterpret_cast<page_head const *>(page_ptr);
    SDL_PAGE_ASSERT(check_page(head, index));
    return head;
}
#else // test only
page_head const *
PagePool::load_page_nolock(pageIndex const index) 
{
    const size_t pageId = index.value(); // uint32 => size_t
    SDL_PAGE_ASSERT(pageId < m.page_count);
    char * const page_ptr = m_alloc.get() + pageId * page_size;
    if (!m_page_commit[pageId]) {
        const size_t offset = pageId * page_size;
        m_file.seekg(offset, std::ios_base::beg);
        m_file.read(page_ptr, page_size);
        m_page_commit[pageId] = true;
    }
    page_head const * const head = reinterpret_cast<page_head const *>(page_ptr);
    SDL_PAGE_ASSERT(check_page(head, index));
    return head;
}
#endif

#if SDL_PAGE_POOL_STAT
void PagePool::page_stat_t::trace() const {
#if SDL_PAGE_POOL_SLOT
    SDL_TRACE("load_page = ", load_page.size(), 
        "/", load_page_request,
        "/", load_slot.size(),
        "\nload_slot:");
    load_slot.trace();
#else
    SDL_TRACE("load_page = ", load_page.size(), 
        "/", load_page_request,
        "/", load_slot.size(),
        "\nload_page:");
    load_page.trace();
#endif
}
#endif

} // pp
} // db
} // sdl
#endif // SDL_TEST_PAGE_POOL