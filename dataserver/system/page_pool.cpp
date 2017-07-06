// page_pool.cpp
//
#include "dataserver/system/page_pool.h"

#if SDL_TEST_PAGE_POOL
namespace sdl { namespace db { namespace pp {

#if SDL_PAGE_POOL_STAT
thread_local PagePool::unique_page_stat
PagePool::thread_page_stat;
#endif

BasePool::BasePool(const std::string & fname)
    : m_file(fname)
{
    throw_error_if_not_t<BasePool>(
        m_file.is_open() && m_file.filesize(), 
        "bad file");
}

PagePool::info_t::info_t(const size_t s)
    : filesize(s)
    , page_count(s / page_size)
    , slot_count((s + slot_size - 1) / slot_size)
{
    SDL_ASSERT(filesize > slot_size);
    SDL_ASSERT(!(filesize % page_size));
    static_assert(is_power_two(slot_page_num), "");
    const size_t n = page_count % slot_page_num;
    last_slot = slot_count - 1;
    last_slot_page_count = n ? n : slot_page_num;
    last_slot_size = page_size * last_slot_page_count;
    SDL_ASSERT(last_slot_size);
}

PagePool::PagePool(const std::string & fname)
    : BasePool(fname)
    , m(m_file.filesize())
{
    SDL_TRACE_FUNCTION;
    A_STATIC_ASSERT_64_BIT;
    static_assert(is_power_two(max_page), "");
    static_assert(is_power_two(slot_size), "");
    static_assert(power_of<slot_size>::value == 16, "");
    static_assert(gigabyte<1>::value / page_size == 131072, "");
    static_assert(gigabyte<5>::value / page_size == 655360, "");
    static_assert(gigabyte<1>::value / slot_size == 16384, "");
    static_assert(gigabyte<5>::value / slot_size == 81920, "");
    SDL_ASSERT((slot_page_num != 8) || (m.slot_count * slot_page_num == m.page_count));
    if (valid_filesize(m.filesize)) {
        m_slot_commit.resize(m.slot_count);
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
        SDL_ASSERT(!(filesize % page_size));
        SDL_ASSERT((slot_page_num != 8) || !(filesize % slot_size));
        return !(filesize % page_size);
    }
    return false;
}

void PagePool::load_all()
{
    SDL_TRACE(__FUNCTION__, " (", m.filesize, ") byte");
    SDL_UTILITY_SCOPE_TIMER_SEC(timer, "load_all seconds = ");
    m_file.read_all(m_alloc.get());
    m_slot_commit.assign(m.slot_count, true);
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

page_head const *
PagePool::load_page_nolock(pageIndex const index) {
    const size_t pageId = index.value(); // uint32 => size_t
    const size_t slotId = pageId / slot_page_num;
    SDL_ASSERT(pageId < m.page_count);
    SDL_ASSERT(slotId < m.slot_count);
#if SDL_PAGE_POOL_STAT
    if (thread_page_stat) {
        thread_page_stat->load_slot.insert((uint32)slotId);
    }
#endif
    char * const page_ptr = m_alloc.get() + pageId * page_size;
    if (!m_slot_commit[slotId]) { //FIXME: should use sequential access to file pages
        char * const slot_ptr = m_alloc.get() + slotId * slot_size;
        if (slotId == m.last_slot) {
            SDL_ASSERT(slot_ptr + m.last_slot_size == m_alloc.get() + m.filesize);
            m_file.read(slot_ptr, slotId * slot_size, m.last_slot_size);
        }
        else {
            m_file.read(slot_ptr, slotId * slot_size, slot_size);
        }
        m_slot_commit[slotId] = true;
    }
    page_head const * const head = reinterpret_cast<page_head const *>(page_ptr);
    SDL_ASSERT(check_page(head, index));
    return head;
}

#if SDL_PAGE_POOL_STAT
void PagePool::page_stat_t::trace() const {
    SDL_TRACE("load_page = ", load_page.size(), 
        "/", load_page_request,
        "/", load_slot.size(),
        "\nload_slot:");
    load_slot.trace();
}
#endif

} // pp
} // db
} // sdl
#endif // SDL_TEST_PAGE_POOL