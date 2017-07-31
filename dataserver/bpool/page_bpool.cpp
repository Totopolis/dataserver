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

lock_page_head::~lock_page_head() {
    if (m_p && !page_bpool::is_zero_block(m_p->pageId())) { // page in memory and locked
        block_head const * const h = block_head::get_block_head(m_p);
        if (h->bpool && !h->is_fixed()) { // bpool can be nullptr if block is fixed in memory
            const_cast<page_bpool *>(h->bpool)->unlock_page(m_p->pageId());
        }
    }
}

block_head *
page_bpool_friend::first_block_head(block32 const blockId) const {
    return m_p->first_block_head(blockId);
}

block_head *
page_bpool_friend::first_block_head(block32 const blockId, freelist const f) const {
    return m_p->first_block_head(blockId, f);
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
    SDL_TRACE("free_pool_block = ", free_pool_block());
}

page_bpool::page_bpool(const std::string & fname,
                       const size_t min_size,
                       const size_t max_size)
    : base_page_bpool(fname, min_size, max_size)
    , init_thread_id(std::this_thread::get_id())
    , m_block(info.block_count)
    , m_thread_id(info.filesize)
    , m_alloc(info.filesize)
    , m_lock_block_list(this, "lock")
    , m_unlock_block_list(this, "unlock")
    , m_free_block_list(this, "free")
    , m_fixed_block_list(this, "fixed")
    , m_td(this)
{
    SDL_TRACE_FUNCTION;
    SDL_ASSERT(max_pool_size <= info.filesize);
    throw_error_if_not_t<page_bpool>(is_open(), "page_bpool");
    load_zero_block();
    if (run_thread) {
        m_td.launch();
    }
}

page_bpool::~page_bpool()
{
}

void page_bpool::load_zero_block()
{
    SDL_ASSERT(thread_fixed(std::this_thread::get_id()) == fixedf::true_);
    char * const block_adr = m_alloc.alloc_block();
    throw_error_if_t<page_bpool>(!block_adr, "bad alloc");
    SDL_ASSERT(block_adr == m_alloc.base());
    m_file.read(block_adr, 0, info.block_size_in_bytes(0));
    m_block[0].set_lock_page_all();
    get_block_head(block_adr, 0)->set_zero_fixed();
}

#if SDL_DEBUG
void page_bpool::trace_block(const char * const title, block32 const blockId, pageIndex const pageId) {
    if (trace_enable) {
        SDL_TRACE(title, "[",page_bpool::realBlock(pageId),"],",blockId,",", pageId,",",page_bit(pageId),
            " L ", m_lock_block_list.head(),
            " U ", m_unlock_block_list.head(),
            " F ", m_free_block_list.head());
    }
}
#endif

page_head const *
page_bpool::lock_block_init(block32 const blockId,
                            pageIndex const pageId,
                            threadId_mask const & threadId,
                            fixedf const fixed) {
#if SDL_DEBUG
    if (trace_enable) {
        trace_block("init", blockId, pageId);
    }
#endif
    SDL_ASSERT(blockId);
    SDL_ASSERT(threadId.second != nullptr);
    char * const block_adr = m_alloc.get_block(blockId);
    char * const page_adr = block_adr + page_head::page_size * page_bit(pageId);
    page_head * const page = reinterpret_cast<page_head *>(page_adr);
    block_head * const first = first_block_head(block_adr);
    { // clear/init block_head for all pages
        first->set_zero();
        for (size_t i = 1, end = info.block_page_count(pageId); i < end; ++i) {
            get_block_head(block_adr, i)->set_zero();
        }
    }
    SDL_DEBUG_CPP(first->blockId = blockId);
    first->realBlock = page_bpool::realBlock(pageId);
    SDL_ASSERT(first->realBlock);
    block_head * const head = block_head::get_block_head(page);
    head->set_lock_thread(threadId.first.value());
    if (is_fixed(fixed)) {
        first->set_fixed();
        m_fixed_block_list.insert(first, blockId);
    }
    else {
        head->set_bpool(this);
        m_lock_block_list.insert(first, blockId);
        threadId.second->set_block(first->realBlock);
    }
    return page;
}

page_head const *
page_bpool::lock_block_head(block32 const blockId,
                            pageIndex const pageId,
                            threadId_mask const & threadId,
                            fixedf const fixed,
                            uint8 const oldLock) {
#if SDL_DEBUG
    if (trace_enable) {
        trace_block("lock", blockId, pageId);
    }
#endif
    SDL_ASSERT(blockId);
    SDL_ASSERT(threadId.second != nullptr);
    char * const block_adr = m_alloc.get_block(blockId);
    char * const page_adr = block_adr + page_head::page_size * page_bit(pageId);
    page_head * const page = reinterpret_cast<page_head *>(page_adr);
    block_head * const first = first_block_head(block_adr);
    SDL_ASSERT(first->blockId == blockId);
    SDL_ASSERT(first->realBlock == page_bpool::realBlock(pageId));
    block_head * const head = block_head::get_block_head(page); // clear/init before first use
    head->set_lock_thread(threadId.first.value());
    if (first->is_fixed()) {
        SDL_ASSERT_DEBUG_2(m_fixed_block_list.find_block(blockId));
        return page;
    }
    else {
        if (is_fixed(fixed)) {
            first->set_fixed();
            if (oldLock) { // was already locked
                m_lock_block_list.remove(first, blockId);
            }
            else { // was unlocked
                m_unlock_block_list.remove(first, blockId);
            }
            m_fixed_block_list.insert(first, blockId);
        }
        else {
            head->set_bpool(this);
            if (oldLock) { // was already locked
                SDL_ASSERT_DEBUG_2(m_lock_block_list.find_block(blockId));
                m_lock_block_list.promote(first, blockId);
            }
            else { // was unlocked
                SDL_ASSERT_DEBUG_2(m_unlock_block_list.find_block(blockId));
                m_unlock_block_list.remove(first, blockId);
                m_lock_block_list.insert(first, blockId);
            }
            threadId.second->set_block(first->realBlock);
        }
    }
    return page;
}

page_bpool::unlock_result
page_bpool::unlock_block_head(block_index & bi,
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
    SDL_ASSERT(threadId.value() < pool_limits::max_thread);
    char * const block_adr = m_alloc.get_block(blockId);
    char * const page_adr = block_adr + page_head::page_size * page_bit(pageId);
    page_head * const page = reinterpret_cast<page_head *>(page_adr); 
    block_head * const head = block_head::get_block_head(page);
    block_head * const first = first_block_head(block_adr);
    if (!head->clr_lock_thread(threadId.value())) { // no more locks for this page
        if (bi.clr_lock_page(page_bit(pageId))) {
            SDL_ASSERT(!bi.is_lock_page(page_bit(pageId))); // this page unlocked
            return unlock_result::false_; // other page(s) is still locked 
        }
        if (first->is_fixed()) {
            SDL_ASSERT_DEBUG_2(m_fixed_block_list.find_block(blockId)); 
            SDL_ASSERT(!first->pageLockThread);
            SDL_ASSERT(!head->pageLockThread);
            return unlock_result::fixed_;
        }
        SDL_ASSERT(head->bpool == this);
        SDL_ASSERT(!head->pageLockThread);
        m_lock_block_list.remove(first, blockId);
        m_unlock_block_list.insert(first, blockId);
        return unlock_result::true_;
    }
    return unlock_result::false_;
}

bool page_bpool::can_alloc_block()
{
    SDL_ASSERT(m_alloc.capacity() >= info.filesize);
    if (max_pool_size < info.filesize) {
        if (m_free_block_list) {
            return true;
        }
        if (m_alloc.used_size() >= max_pool_size) {
            SDL_WARNING(!"low on memory");
            if (free_unlock_blocks(free_pool_block())) {
                SDL_ASSERT(m_free_block_list);
                return true;
            }
            else {
                SDL_WARNING_DEBUG_2(!"low on memory");
            }
        }
        if (m_alloc.can_alloc(pool_limits::block_size)) {
            return true;
        }
        SDL_ASSERT(!"can_alloc");
        return false;
    }
    SDL_ASSERT(m_alloc.can_alloc(pool_limits::block_size));
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
            p.first->set_zero(); // prepare block to reuse
            page_head * const page_adr = block_head::get_page_head(p.first);
            SDL_ASSERT(m_alloc.get_block(p.second) == reinterpret_cast<char *>(page_adr));
            return reinterpret_cast<char *>(page_adr);
        }
        return m_alloc.alloc_block();
    }
    SDL_ASSERT(!"bad alloc");
    return nullptr;
}

page_head const *
page_bpool::lock_page(pageIndex const pageId)
{
    const uint32 real_blockId = page_bpool::realBlock(pageId);
    SDL_ASSERT(real_blockId < m_block.size());
    if (!real_blockId) { // zero block must be always in memory
        page_head const * const page = zero_block_page(pageId);
        SDL_ASSERT(page->valid_checksum());
        SDL_ASSERT(!get_block_head(real_blockId, pageId)->pageLockThread);
        return page;
    }
    if (info.last_block < real_blockId) {
        throw_error_t<block_index>("page not found");
        return nullptr;
    }
    auto const this_thread = std::this_thread::get_id();
    lock_guard lock(m_mutex); // should be improved
    thread_id_t::pos_mask thread_index(m_thread_id.insert(this_thread)); // throw if too many threads
    SDL_ASSERT(thread_index.second != nullptr);
    block_index & bi = m_block[real_blockId];
    if (bi.blockId()) { // block is loaded
        if (page_head const * const page = lock_block_head(bi.blockId(), pageId, thread_index,
            thread_fixed(this_thread),
            bi.set_lock_page(page_bit(pageId)))) {
            SDL_ASSERT_DEBUG_2(page->valid_checksum());
            SDL_ASSERT(bi.pageLock());
            SDL_DEBUG_CPP(auto const test = get_block_head(bi.blockId(), pageId));
            SDL_ASSERT(test->is_lock_thread(thread_index.first.value()));
            return page;
        }
    }
    else { // block is NOT loaded
        if (char * const block_adr = alloc_block()) {
            read_block_from_file(block_adr, real_blockId);
            block32 const allocId = m_alloc.block_id(block_adr);
            SDL_ASSERT(m_alloc.get_block(allocId) == block_adr);
            bi.set_blockId(allocId);
            bi.set_lock_page(page_bit(pageId));
            if (page_head const * const page = lock_block_init(allocId, pageId, thread_index,
                thread_fixed(this_thread))) {
                SDL_ASSERT_DEBUG_2(page->valid_checksum());
                SDL_ASSERT(bi.pageLock());
                SDL_DEBUG_CPP(auto const test = get_block_head(bi.blockId(), pageId));
                SDL_ASSERT(test->is_lock_thread(thread_index.first.value()));
                return page;
            }
        }
    }
    SDL_ASSERT(0);
    throw_error_t<block_index>("bad alloc");
    return nullptr;
}

bool page_bpool::unlock_page(pageIndex const pageId)
{
    SDL_ASSERT(pageId.value() < info.page_count);
    const uint32 real_blockId = page_bpool::realBlock(pageId);
    SDL_ASSERT(real_blockId < m_block.size());
    if (!real_blockId) { // zero block must be always in memory
        return false;
    }
    if (info.last_block < real_blockId) {
        throw_error_t<block_index>("page not found");
        return false;
    }
    lock_guard lock(m_mutex); // should be improved
    block_index & bi = m_block[real_blockId];
    if (bi.blockId()) { // block is loaded
        if (bi.is_lock_page(page_bit(pageId))) {
            auto const this_thread = std::this_thread::get_id();
            threadId_mask thread_index = m_thread_id.find(this_thread);
            if (!thread_index.second) { // thread NOT found
                SDL_ASSERT(!"thread NOT found"); // to be tested
                return false;
            }
            SDL_DEBUG_CPP(auto const test = get_block_head(bi.blockId(), pageId));
            const unlock_result result = unlock_block_head(bi, bi.blockId(), pageId, thread_index.first);
            if (result == unlock_result::true_) { // block is NOT used
                SDL_ASSERT(!bi.pageLock());
                SDL_ASSERT(thread_index.second->is_block(real_blockId));
                thread_index.second->clr_block(real_blockId);
                SDL_ASSERT(!test->pageLockThread);
                return true;
            }
            else { // block is used or fixed
                SDL_ASSERT((result == unlock_result::fixed_) || bi.pageLock());
                SDL_ASSERT(!test->is_lock_thread(thread_index.first.value()));
                if (thread_index.second->is_block(real_blockId)) {
                    char * const block_adr = m_alloc.get_block(bi.blockId());
                    const size_t count = info.block_page_count(pageId);
                    for (size_t i = 1; i < count; ++i) { // can this thread release block ?
                        block_head const * const h = get_block_head(block_adr, i);
                        if (h->is_lock_thread(thread_index.first.value())) {
                            return false;
                        }
                    }
                    thread_index.second->clr_block(real_blockId);
                }
                return false;
            }
        }
    }
    return false;
}

size_t page_bpool::free_unlocked(bool const decommit) // returns blocks number
{
    lock_guard lock(m_mutex);
    const size_t size = free_unlock_blocks(info.block_count);
    if (decommit && m_free_block_list) {
        if (m_alloc.decommit(m_free_block_list)) {
            SDL_ASSERT(!m_free_block_list);
            SDL_ASSERT(m_alloc.can_alloc(size * pool_limits::block_size));
        }
    }
    return size;
}

// mutex already locked
bool page_bpool::thread_unlock_page(threadIndex const thread_index, pageIndex const pageId)
{
    SDL_ASSERT(!is_init_thread());
    SDL_ASSERT(pageId.value() < info.page_count);
    const size_t real_blockId = page_bpool::realBlock(pageId);
    if (!real_blockId) { // zero block must be always in memory
        SDL_ASSERT(0);
        return false;
    }
    if (info.last_block < real_blockId) {
        throw_error_t<block_index>("page not found");
        return false;
    }
    block_index & bi = m_block[real_blockId];
    if (bi.blockId()) { // block is loaded
        if (bi.is_lock_page(page_bit(pageId))) {
            SDL_DEBUG_CPP(auto const test = get_block_head(bi.blockId(), pageId));
            SDL_ASSERT(test->pageLockThread);
            const unlock_result result = unlock_block_head(bi, bi.blockId(), pageId, thread_index);
            if (result == unlock_result::true_) {
                SDL_ASSERT(!bi.pageLock());
                SDL_ASSERT(!test->pageLockThread);
                return true;
            }
            else { // block is used or fixed
                SDL_ASSERT(bi.pageLock());
                SDL_ASSERT(!test->is_lock_thread(thread_index.value()));
                return false;
            }
        }
    }
    return false;
}

// mutex already locked
bool page_bpool::thread_unlock_block(threadIndex const thread_index, size_t const blockId)
{
    SDL_ASSERT(blockId);
    const size_t count = info.block_page_count(blockId);
    for (size_t i = 0; i < count; ++i) {
        pageIndex const pageId = static_cast<page32>(blockId * pool_limits::block_page_num + i);
        if (thread_unlock_page(thread_index, pageId)) {
            return true;
        }
    }
    return false;
}

size_t page_bpool::unlock_thread(std::thread::id const id, const bool remove_id) 
{
    if (is_init_thread(id)) {
        SDL_ASSERT(!"unlock_thread");
        return 0;
    }
    SDL_TRACE_IF(trace_enable, "* unlock_thread ", id);
    lock_guard lock(m_mutex); // should be improved
    threadId_mask thread_index = m_thread_id.find(id);
    if (!thread_index.second) { // thread NOT found
        SDL_WARNING_DEBUG_2(0);
        return 0;
    }
    size_t unlock_count = 0;
    thread_mask_t & mask = *(thread_index.second);
    mask.for_each_block([this, &thread_index, &unlock_count](size_t const blockId){
        SDL_ASSERT(blockId);
        if (blockId && thread_unlock_block(thread_index.first, blockId)) {
            ++unlock_count;
        }
    });
    if (remove_id) {
        m_thread_id.erase(id);
    }
    else {
        mask.clear();
    }
    SDL_TRACE_IF(trace_enable, "* unlock_thread ", id, " blocks = ", unlock_count);
    return unlock_count;
}

size_t page_bpool::free_unlock_blocks(size_t const block_count)
{
    if (!block_count) {
        SDL_ASSERT(0);
        return 0;
    }
    SDL_ASSERT(block_count <= info.block_count);
    SDL_ASSERT(m_unlock_block_list.assert_list());
    block_list_t free_block_list(this);
    size_t const free_count = m_unlock_block_list.truncate(free_block_list, block_count);
    if (free_count) {
        SDL_ASSERT(free_block_list);
        free_block_list.for_each([this](block_head * const h, block32 const p){
            SDL_ASSERT(h->realBlock);
            SDL_ASSERT(!h->is_fixed());
            SDL_ASSERT(p);
            block_index & bi = m_block[h->realBlock];
            SDL_ASSERT(!bi.pageLock());
            SDL_ASSERT(bi.blockId() == p);
            bi.clr_blockId(); // must be reused
            h->realBlock = block_list_t::null;
        }, freelist::false_);
        m_free_block_list.append(std::move(free_block_list), freelist::true_);
        SDL_ASSERT_DEBUG_2(m_unlock_block_list.assert_list(freelist::false_));
        SDL_ASSERT_DEBUG_2(m_free_block_list.assert_list(freelist::true_));
        SDL_ASSERT(m_free_block_list);
        return free_count;
    }
    SDL_ASSERT(m_unlock_block_list.empty());
    return 0;
}

#if SDL_DEBUG
void page_bpool::trace_free()
{
    lock_guard lock(m_mutex);
    m_free_block_list.trace(freelist::true_);
}
#endif

//---------------------------------------------------

page_bpool::thread_data::thread_data(page_bpool * const p)
    : m_parent(p)
    , m_shutdown(false)
    , m_ready(false)
#if defined(SDL_OS_WIN32) || SDL_DEBUG
    , m_period(1)
#else
    , m_period(30)
#endif
{
    SDL_ASSERT(m_parent);
}

page_bpool::thread_data::~thread_data(){
    if (m_thread) {
        shutdown();
    }
}

void page_bpool::thread_data::launch() {
    SDL_TRACE_FUNCTION;
    SDL_ASSERT(!m_thread);
    m_thread.reset(new joinable_thread([this](){
        this->run_thread();
    }));
}

void page_bpool::thread_data::shutdown(){
    m_shutdown = true;
    m_ready = true;
    m_cv.notify_one();
}

void page_bpool::thread_data::notify() {
    m_ready = true;
    m_cv.notify_one();
}

void page_bpool::thread_data::run_thread()
{
    SDL_ASSERT(!m_shutdown);
    SDL_ASSERT(!m_ready);
    bool timeout = false;
    while (!m_shutdown) {
        {
            std::unique_lock<std::mutex> lock(m_cv_mutex);
            m_cv.wait_for(lock, std::chrono::seconds(m_period), [this]{
                return m_ready.load();
            });
            timeout = !m_ready;
            m_ready = false;
        }
        SDL_TRACE("~");
        (void)timeout;
        //async MEM_DECOMMIT
    }
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

#if 0
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
#endif