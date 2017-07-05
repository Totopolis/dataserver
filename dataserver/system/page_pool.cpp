// page_pool.cpp
//
#include "dataserver/system/page_pool.h"

#if SDL_TEST_PAGE_POOL
namespace sdl { namespace db { namespace pp {

#if SDL_PAGE_POOL_STAT
thread_local PagePool::unique_page_stat
PagePool::thread_page_stat;
#endif

PagePool::PagePool(const std::string & fname)
    : m_file(fname, std::ifstream::in | std::ifstream::binary)
{
    SDL_TRACE_FUNCTION;
    static_assert(is_power_two(max_page), "");
    static_assert(is_power_two(slot_size), "");
    static_assert(power_of<slot_size>::value == 16, "");
    static_assert(gigabyte<1>::value / slot_size == 16384, "");
    throw_error_if_not<this_error>(m_file.is_open(), "file not found");
    m_file.seekg(0, std::ios_base::end);
    m.filesize = m_file.tellg();
    m.page_count = m.filesize / page_size;
    m.slot_count = m.filesize / slot_size;
    SDL_ASSERT(m.slot_count * slot_page == m.page_count);
    if (valid_filesize(m.filesize)) {
#if SDL_PAGE_POOL_SLOT
        m_slot_commit.resize(m.slot_count);
#else
        m_page_commit.resize(m.page_count);
#endif
        m_alloc.reset(new char[m.filesize]);
        throw_error_if_not<this_error>(is_open(), "bad alloc");
        if (0) {
            load_all();
        }
    }
    else {
        throw_error<this_error>("bad alloc size");
    }
}

bool PagePool::valid_filesize(const size_t filesize)
{
    if (filesize > slot_size) {
        SDL_ASSERT(!(filesize % page_size));
        SDL_ASSERT(!(filesize % slot_size));
        return !(filesize % page_size) && !(filesize % slot_size);
    }
    return false;
}

void PagePool::load_all()
{
    SDL_TRACE_FUNCTION;
    SDL_DEBUG_CODE(SDL_UTILITY_SCOPE_TIMER_SEC(timer, "load_all seconds = ");)
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
    SDL_ASSERT(pageId < m.page_count);
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
    const size_t slotId = pageId / slot_page;
    SDL_ASSERT(pageId < m.page_count);
    SDL_ASSERT(slotId < m.slot_count);
#if SDL_PAGE_POOL_STAT
    if (thread_page_stat) {
        thread_page_stat->load_slot.insert((uint32)slotId);
    }
#endif
    char * const page_ptr = m_alloc.get() + pageId * page_size;
    if (!m_slot_commit[slotId]) {
        //FIXME: suboptimal, should use sequential access to file pages
        char * const slot_ptr = m_alloc.get() + slotId * slot_size;
        const size_t offset = slotId * slot_size;
        m_file.seekg(offset, std::ios_base::beg);
        m_file.read(slot_ptr, slot_size);
        m_slot_commit[slotId] = true;
        SDL_DEBUG_CODE(++m_slot_loaded);
    }
    page_head const * const head = reinterpret_cast<page_head const *>(page_ptr);
    SDL_ASSERT(assert_page(head, pageId));
    return head;
}
#else
page_head const *
PagePool::load_page_nolock(pageIndex const index) 
{
    const size_t pageId = index.value(); // uint32 => size_t
    SDL_ASSERT(pageId < m.page_count);
    char * const page_ptr = m_alloc.get() + pageId * page_size;
    if (!m_page_commit[pageId]) {
        //FIXME: suboptimal, should use sequential access to file pages
        const size_t offset = pageId * page_size;
        m_file.seekg(offset, std::ios_base::beg);
        m_file.read(page_ptr, page_size);
        m_page_commit[pageId] = true;
        SDL_DEBUG_CODE(++m_page_loaded;)
    }
    page_head const * const head = reinterpret_cast<page_head const *>(page_ptr);
    SDL_ASSERT(assert_page(head, pageId));
    return head;
}
#endif

} // pp
} // db
} // sdl
#endif // SDL_TEST_PAGE_POOL