// page_bpool.cpp
//
#include "dataserver/bpool/page_bpool.h"

namespace sdl { namespace db { namespace bpool {

pool_info_t::pool_info_t(const size_t s)
    : filesize(s)
    , page_count(s / T::page_size)
    , block_count((s + T::block_size - 1) / T::block_size)
{
    SDL_ASSERT(filesize > T::block_size);
    SDL_ASSERT(!(filesize % T::page_size));
    static_assert(is_power_two(T::block_page_num), "");
    const size_t n = page_count % T::block_page_num;
    last_block = block_count - 1;
    last_block_page_count = n ? n : T::block_page_num;
    last_block_size = T::page_size * last_block_page_count;
    SDL_ASSERT((last_block_size >= T::page_size) && (last_block_size <= T::block_size));
}

//---------------------------------------------------

page_bpool_file::page_bpool_file(const std::string & fname)
    : m_file(fname) 
{
    throw_error_if_not_t<base_page_bpool>(m_file.is_open() && m_file.filesize(), "bad file");
    throw_error_if_not_t<base_page_bpool>(valid_filesize(m_file.filesize()), "bad filesize");
    throw_error_if_not_t<base_page_bpool>(m_file.filesize() <= pool_limits::max_filesize, "max_filesize");
}

bool page_bpool_file::valid_filesize(const size_t filesize) {
    using T = pool_limits;
    if (filesize > T::block_size) {
        if (!(filesize % T::page_size)) {
            return true;
        }
    }
    SDL_ASSERT(0);
    return false;
}

//------------------------------------------------------

base_page_bpool::base_page_bpool(const std::string & fname, 
                                 const size_t min_size,
                                 const size_t max_size)
    : page_bpool_file(fname)
    , info(filesize())
    , min_pool_size(min_size ? a_min(min_size, filesize()) : filesize())
    , max_pool_size(max_size ? a_min(max_size, filesize()) : filesize())
{
    SDL_ASSERT(min_size <= max_size);
    SDL_ASSERT(min_pool_size);
    SDL_ASSERT(min_pool_size <= max_pool_size);
    const size_t size = (max_pool_size - min_pool_size) / pool_limits::block_size * pool_limits::block_size;
    m_free_pool_size = a_max(size, (size_t)pool_limits::block_size * 2);
    SDL_ASSERT(!(m_free_pool_size % pool_limits::block_size));
    SDL_TRACE("free_pool_size = ", m_free_pool_size / pool_limits::block_size, " blocks");
}

page_bpool::page_bpool(const std::string & fname,
                       const size_t min_size,
                       const size_t max_size)
    : base_page_bpool(fname, min_size, max_size)
    , m_block(info.block_count)
    , m_thread_id(info.filesize)
    , m_alloc(info.filesize)
    , m_lock_block_list(this, "lock")
    , m_unlock_block_list(this, "unlock")
    , m_free_block_list(this, "free")
{
    SDL_TRACE_FUNCTION;
    SDL_ASSERT(max_pool_size <= info.filesize);
    throw_error_if_not_t<page_bpool>(is_open(), "page_bpool");
    load_zero_block();
}

page_bpool::~page_bpool()
{
}

void page_bpool::load_zero_block()
{
    char * const block_adr = m_alloc.alloc(pool_limits::block_size);
    if (block_adr) {
        SDL_ASSERT(block_adr == m_alloc.base());
        m_file.read(block_adr, 0, info.block_size_in_bytes(0));
        m_block[0].set_lock_page_all();
    }
    else {
        throw_error_t<page_bpool>("bad alloc");
    }
}

#if SDL_DEBUG
void page_bpool::trace_block(const char * const title, block32 const blockId, pageIndex const pageId) {
    if (trace_enable) {
        SDL_TRACE(title, "[",realBlock(pageId),"],",blockId,",", pageId,",",page_bit(pageId),
            " L ", m_lock_block_list.head(),
            " U ", m_unlock_block_list.head(),
            " F ", m_free_block_list.head());
    }
}
#endif

page_head const *
page_bpool::lock_block_init(block32 const blockId,
                            pageIndex const pageId,
                            threadIndex const thread) {
#if SDL_DEBUG
    if (trace_enable) {
        trace_block("init", blockId, pageId);
    }
#endif
    SDL_ASSERT(blockId);
    char * const block_adr = m_alloc.get_block(blockId);
    char * const page_adr = block_adr + page_head::page_size * page_bit(pageId);
    page_head * const page = reinterpret_cast<page_head *>(page_adr);
    block_head * const first = first_block_head(block_adr);
    {
        // clear/init block_head for all pages
        SDL_ASSERT(valid_checksum(page, pageId));
        SDL_ASSERT(memcmp_zero(*first) || !valid_checksum(block_adr, block_pageIndex(pageId)));
        memset_zero(*first);
        const size_t count = info.block_page_count(pageId);
        for (size_t i = 1; i < count; ++i) {
            block_head * const h = get_block_head(block_adr, i);
            SDL_DEBUG_CPP(const auto test = block_pageIndex(page->data.pageId.pageId, i));
            SDL_ASSERT(memcmp_zero(*h) || !valid_checksum(get_block_page(block_adr, i), test));
            memset_zero(*h);
        }
    }
    SDL_DEBUG_CPP(first->blockId = blockId);
    first->realBlock = realBlock(pageId);
    SDL_ASSERT(first->realBlock);
    SDL_DEBUG_CPP(first->blockInitTime = unix_time());
    {
        block_head * const h = get_block_head(page);
        h->set_lock_thread(thread.value());
        h->pageAccessTime = pageAccessTime();
    }
    SDL_ASSERT_DEBUG_2(!m_lock_block_list.find_block(blockId));
    SDL_ASSERT_DEBUG_2(!m_unlock_block_list.find_block(blockId));
    m_lock_block_list.insert(first, blockId);
    SDL_ASSERT(page->data.pageId.pageId == pageId.value());
    return page;
}

page_head const *
page_bpool::lock_block_head(block32 const blockId,
                            pageIndex const pageId,
                            threadIndex const threadId,
                            int const oldLock) {
#if SDL_DEBUG
    if (trace_enable) {
        trace_block("lock", blockId, pageId);
    }
#endif
    SDL_ASSERT(blockId);
    char * const block_adr = m_alloc.get_block(blockId);
    char * const page_adr = block_adr + page_head::page_size * page_bit(pageId);
    page_head * const page = reinterpret_cast<page_head *>(page_adr);
    block_head * const first = first_block_head(block_adr);
    SDL_ASSERT(first->blockId == blockId);
    {
        block_head * const h = get_block_head(page);
        SDL_ASSERT(h->pageAccessTime || h->blockInitTime || memcmp_zero(*h)); // must be cleared before first use
        h->set_lock_thread(threadId.value());
        h->pageAccessTime = pageAccessTime();
    }
    SDL_ASSERT_DEBUG_2(m_lock_block_list.find_block(blockId) == !!oldLock);
    SDL_ASSERT_DEBUG_2(m_unlock_block_list.find_block(blockId) == !oldLock);
    if (oldLock) { // was already locked
        m_lock_block_list.promote(first, blockId);
    }
    else { // was unlocked
        m_unlock_block_list.remove(first, blockId);
        m_lock_block_list.insert(first, blockId);
    }
    SDL_ASSERT(page->data.pageId.pageId == pageId.value());
    return page;
}

bool page_bpool::unlock_block_head(block_index & bi,
                                   block32 const blockId,
                                   pageIndex const pageId, 
                                   threadIndex const threadId) {
#if SDL_DEBUG
    if (trace_enable) {
        trace_block("unlock", blockId, pageId);
    }
#endif
    SDL_ASSERT(blockId);
    SDL_ASSERT(bi.pageLock());
    char * const block_adr = m_alloc.get_block(blockId);
    char * const page_adr = block_adr + page_head::page_size * page_bit(pageId);
    page_head * const page = reinterpret_cast<page_head *>(page_adr); 
    block_head * const head = get_block_head(page);
    SDL_ASSERT_DEBUG_2(m_lock_block_list.find_block(blockId)); 
    SDL_ASSERT_DEBUG_2(!m_unlock_block_list.find_block(blockId));
    if (!head->clr_lock_thread(threadId.value())) { // no more locks for this page
        if (bi.clr_lock_page(page_bit(pageId))) {
            SDL_ASSERT(!bi.is_lock_page(page_bit(pageId))); // this page unlocked
            return false; // other page is still locked 
        }        
        block_head * const first = first_block_head(block_adr);
        m_lock_block_list.remove(first, blockId);
        m_unlock_block_list.insert(first, blockId);
        return true;
    }
    return false;
}

// must be called only for allocated pages and before page_head.reserved is modified
bool page_bpool::valid_checksum(page_head const * const head, const pageIndex pageId) {
    if (head->data.pageId.pageId == pageId.value()) {
        if (!head->data.tornBits || head->valid_checksum()) {
            SDL_ASSERT(memcmp_zero(head->data.reserved) && "warning");
            return true;
        }
    }
    return false;
}

bool page_bpool::valid_checksum(char const * const block_adr, const pageIndex pageId) {
    char const * const page_adr = block_adr + page_head::page_size * page_bit(pageId);
    page_head const * const head = reinterpret_cast<page_head const *>(page_adr);
    return valid_checksum(head, pageId);
}

bool page_bpool::can_alloc_block()
{
    if (max_pool_size < info.filesize) {
        SDL_ASSERT(m_alloc.capacity() == max_pool_size);
        if (m_free_block_list) {
            return true;
        }
        if (m_alloc.can_alloc(pool_limits::block_size)) {
            return true;
        }
        if (free_unlock_blocks(free_pool_size())) {
            return true;
        }
        SDL_WARNING(!"low on memory");
        return false;
    }
    SDL_ASSERT(m_alloc.capacity() == info.filesize);
    SDL_ASSERT(m_alloc.can_alloc(pool_limits::block_size));
    SDL_ASSERT(!m_free_block_list); // no memory leaks allowed
    return true;
}

char * page_bpool::alloc_block()
{
    if (can_alloc_block()) {
        if (m_free_block_list) { // must reuse free block (memory already allocated)
            auto p = m_free_block_list.pop_head(freelist::true_);
            SDL_ASSERT(p.first && p.second);
            SDL_ASSERT(p.first->blockId == p.second);
            A_STATIC_CHECK_TYPE(block_head *, p.first);
            memset_zero(*p.first); // prepare block to reuse
            page_head * const page_adr = get_page_head(p.first);
            SDL_ASSERT(m_alloc.get_block(p.second) == reinterpret_cast<char *>(page_adr));
            return reinterpret_cast<char *>(page_adr);
        }
        return m_alloc.alloc(pool_limits::block_size);
    }
    SDL_ASSERT(!"bad alloc");
    return nullptr;
}

page_head const *
page_bpool::lock_page(pageIndex const pageId)
{
    const size_t real_blockId = realBlock(pageId);
    SDL_ASSERT(real_blockId < m_block.size());
    if (!real_blockId) { // zero block must be always in memory
        SDL_ASSERT(valid_checksum(m_alloc.base(), pageId));
        return zero_block_page(pageId);
    }
    if (info.last_block < real_blockId) {
        throw_error_t<block_index>("page not found");
        return nullptr;
    }
    lock_guard lock(m_mutex); // should be improved
    threadIndex const threadId(m_thread_id.insert().first);
    block_index & bi = m_block[real_blockId];
    if (bi.blockId()) { // block is loaded
        return lock_block_head(bi.blockId(), pageId, threadId,
            bi.set_lock_page(page_bit(pageId)));
    }
    else { // block is NOT loaded
        if (char * const block_adr = alloc_block()) {
            read_block_from_file(block_adr, real_blockId);
            SDL_ASSERT(valid_checksum(block_adr, pageId));
            block32 const allocId = m_alloc.block_id(block_adr);
            SDL_ASSERT(m_alloc.get_block(allocId) == block_adr);
            bi.set_blockId(allocId);
            bi.set_lock_page(page_bit(pageId));
            return lock_block_init(allocId, pageId, threadId);
        }
    }
    SDL_ASSERT(0);
    throw_error_t<block_index>("bad alloc");
    return nullptr;
}

bool page_bpool::unlock_page(pageIndex const pageId)
{
    const size_t blockId = realBlock(pageId);
    SDL_ASSERT(blockId < m_block.size());
    if (!blockId) { // zero block must be always in memory
        return false;
    }
    if (info.last_block < blockId) {
        throw_error_t<block_index>("page not found");
        return false;
    }
    lock_guard lock(m_mutex); // should be improved
    const auto thread_id = m_thread_id.find();
    if (!thread_id.second) { // thread NOT found
        SDL_ASSERT(0);
        return false;
    }
    block_index & bi = m_block[blockId];
    if (bi.blockId()) { // block is loaded
        if (bi.is_lock_page(page_bit(pageId))) {
            if (unlock_block_head(bi, bi.blockId(), pageId, thread_id.first)) {
                SDL_ASSERT(!bi.pageLock());
                return true;
            }
            SDL_ASSERT(bi.pageLock());
        }
    }
    return false;
}

size_t page_bpool::free_unlock_blocks(size_t const memory)
{
    SDL_ASSERT(memory && !(memory % pool_limits::block_size));
    const size_t block_count = memory / pool_limits::block_size;
    SDL_ASSERT(block_count);
    SDL_ASSERT((block_count > 1) && "free_pool_size");
    SDL_ASSERT(m_unlock_block_list.assert_list());
    block_list_t free_block_list(this);
    size_t const free_count = m_unlock_block_list.truncate(free_block_list, block_count);
    if (free_count) {
        SDL_ASSERT(free_block_list);
        free_block_list.for_each([this](block_head * const h, block32 const p){
            SDL_ASSERT(h->realBlock);
            SDL_ASSERT(p);
            block_index & bi = m_block[h->realBlock];
            SDL_ASSERT(!bi.pageLock());
            SDL_ASSERT(bi.blockId() == p);
            bi.clr_blockId(); // must be reused
            h->realBlock = block_list_t::null;
            return true;
        }, freelist::false_);
        m_free_block_list.append(std::move(free_block_list), freelist::true_);
        SDL_ASSERT(m_unlock_block_list.assert_list(freelist::false_));
        SDL_ASSERT(m_free_block_list.assert_list(freelist::true_));
        return free_count;
    }
    SDL_WARNING_DEBUG_2(!"low on memory");
    return 0;
}

uint32 page_bpool::lastAccessTime(block32 const b) const
{
    uint32 pageAccessTime = 0;
    char * const block_adr = m_alloc.get_block(b);
    size_t const count = info.block_page_count(b);
    for (size_t i = 0; i < count; ++i) {
        block_head const * const h = get_block_head(get_block_page(block_adr, i));
        set_max(pageAccessTime, h->pageAccessTime);
    }
    SDL_ASSERT(pageAccessTime);
    return pageAccessTime;
}

#if SDL_DEBUG
namespace {
    class unit_test {
    public:
        unit_test() {
        }
    };
    static unit_test s_test;
}
#endif //#if SDL_DEBUG
}}} // sdl
