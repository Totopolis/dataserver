// page_pool.cpp
//
#include "dataserver/system/page_pool.h"

#if SDL_TEST_PAGE_POOL
namespace sdl { namespace db { namespace pp {

#if SDL_PAGE_POOL_STAT
thread_local PagePool::unique_page_stat
PagePool::thread_page_stat;
#endif

BasePool::BasePool(const std::string & fname): m_file(fname) 
{
    throw_error_if_not_t<BasePool>(m_file.is_open() && m_file.filesize(), "bad file");
    throw_error_if_not_t<BasePool>(valid_filesize(m_file.filesize()), "bad alloc size");
}

bool BasePool::valid_filesize(const size_t filesize) {
    if (filesize > slot_size) {
        SDL_ASSERT(!(filesize % page_size));
        SDL_ASSERT((slot_page_num != 8) || !(filesize % slot_size));
        return !(filesize % page_size);
    }
    return false;
}

PagePool::info_t::info_t(const size_t s)
    : max_pool_size(s)
    , filesize(s)
    , page_count(s / page_size)
    , slot_count((s + slot_size - 1) / slot_size)
{
    SDL_ASSERT(filesize > slot_size);
    SDL_ASSERT(!(filesize % page_size));
    static_assert(is_power_two(slot_page_num), "");
    {
        const size_t n = page_count % slot_page_num;
        last_slot = slot_count - 1;
        last_slot_page_count = n ? n : slot_page_num;
        last_slot_size = page_size * last_slot_page_count;
        SDL_ASSERT(last_slot_size && (last_slot_size <= slot_size));
    }
}

PagePool::PagePool(const std::string & fname)
    : BasePool(fname)
    , m_info(m_file.filesize())
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
    static_assert(gigabyte<500>::value / slot_size == 8192000, "");
    SDL_ASSERT((slot_page_num != 8) || (m_info.slot_count * slot_page_num == m_info.page_count));
    m_alloc.reset(new vm_alloc(m_file.filesize(), commit_all));
    throw_error_if_not<this_error>(m_alloc->is_open(), "bad alloc");
    m_slot_load.data().resize(m_info.slot_count);
#if SDL_PAGE_POOL_LOAD_ALL
    load_all();
#endif
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

void PagePool::load_all() {
    SDL_TRACE(__FUNCTION__, " (", m_info.filesize, ") byte");
    SDL_UTILITY_SCOPE_TIMER_SEC(timer, "load_all seconds = ");
    m_file.read_all(m_alloc->alloc_all());
    m_slot_load.data().assign(m_info.slot_count, true);
}

page_head const *
PagePool::load_page(pageIndex const index) {
    const size_t pageId = index.value(); // uint32 => size_t
    const size_t slotId = pageId / slot_page_num;
    SDL_ASSERT(pageId < m_info.page_count);
    SDL_ASSERT(slotId < m_info.slot_count);
    if (pageId >= page_count()) {
        throw_error<this_error>("bad page");
        return nullptr;
    }
#if SDL_PAGE_POOL_STAT
    if (thread_page_stat) {
        thread_page_stat->load_page_request++;
        thread_page_stat->load_page.insert((uint32)pageId);
        thread_page_stat->load_slot.insert((uint32)slotId);
    }
#endif
    char * const base_address = m_alloc->base_address();
    char * const page_ptr = base_address + pageId * page_size;
    if (!m_slot_load[slotId]) { // use spinlock
        lock_guard lock(m_mutex);
        if (!m_slot_load[slotId]) { // must check again after mutex lock
            char * const slot_ptr = base_address + slotId * slot_size;
            if (commit_all || m_alloc->alloc(slot_ptr, m_info.alloc_slot_size(slotId))) {
                m_file.read(slot_ptr, slotId * slot_size, slot_size);
                m_slot_load.set_true(slotId);
            }
        }
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