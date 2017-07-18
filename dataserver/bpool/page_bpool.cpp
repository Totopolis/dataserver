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

#if 0
bit_info_t::bit_info_t(pool_info_t const & in)
    : bit_size((in.block_count + 7) / 8)
{
    SDL_ASSERT(in.block_count);
    byte_size = (bit_size + 7) / 8;
    last_byte = byte_size - 1;
    const size_t n = bit_size % 8;
    last_byte_bits = n ? n : 8;
    SDL_ASSERT(bit_size && byte_size && last_byte_bits);
}
#endif

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

thread_id_t::size_bool
thread_id_t::insert(id_type const id)
{
    const size_bool ret = algo::unique_insertion_distance(m_data, id);
    if (ret.second) {
        if (size() > max_thread) {
            SDL_ASSERT(!"max_thread");
            this->erase(id);
            SDL_ASSERT(size() == max_thread);
            throw_error_t<thread_id_t>("too many threads");
            return { max_thread, false };
        }
    }
    SDL_ASSERT(size() <= max_thread);
    return ret;
}

thread_id_t::size_bool
thread_id_t::find(id_type const id) const
{
    const auto pos = algo::binary_find(m_data, id);
    const size_t d = std::distance(m_data.begin(), pos);
    const bool found = (pos != m_data.end());
    SDL_ASSERT(found == (d < size()));
    return { d, found };
}

bool thread_id_t::erase(id_type const id)
{
    return algo::binary_erase(m_data, id);
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
}

page_bpool::page_bpool(const std::string & fname,
                       const size_t min_size,
                       const size_t max_size)
    : base_page_bpool(fname, min_size, max_size)
    , m_block(info.block_count)
    , m_alloc(base_page_bpool::max_pool_size)
{
    SDL_TRACE_FUNCTION;
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

page_head const *
page_bpool::lock_block_head(block32 const blockId, pageIndex const pageId, const size_t thread_id,
    bool const initialize) {
    SDL_ASSERT(blockId);
    char * const block_adr = m_alloc.get_block(blockId);
    char * const page_adr = block_adr + page_head::page_size * page_bit(pageId);
    page_head * const page = reinterpret_cast<page_head *>(page_adr); 
    block_head * const head = get_block_head(page);
    if (initialize) {
        SDL_ASSERT(memcmp_zero(*head));
        memset_zero(*head);
    }
    head->set_lock_thread(thread_id);
    head->pageAccessTime = ++m_accessCnt;
    if (adaptive_block_list) { // update used/unused block list...
        if (m_lock_block_list) {
            //SDL_ASSERT(0);
        }
        else {
            m_lock_block_list = blockId;             
            head->prevBlock = 0;
            head->nextBlock = 0;
        }
        if (m_unlock_block_list) {
            //
        }
    }
    SDL_ASSERT(page->data.pageId.pageId == pageId.value());
    return page;
}

bool page_bpool::unlock_block_head(block32 const blockId, pageIndex const pageId, const size_t thread_id) {
    SDL_ASSERT(blockId);
    char * const block_adr = m_alloc.get_block(blockId);
    char * const page_adr = block_adr + page_head::page_size * page_bit(pageId);
    page_head * const page = reinterpret_cast<page_head *>(page_adr); 
    block_head * const head = get_block_head(page);
    if (!head->clr_lock_thread(thread_id)) {
        if (adaptive_block_list) { // update used/unused block list...
            if (m_unlock_block_list) {
                //SDL_ASSERT(0);
            }
            else {
                m_unlock_block_list = blockId;
                head->prevBlock = 0;
                head->nextBlock = 0;
            }
            if (m_lock_block_list) {
            }
        }
        return true;
    }
    return false;
}

#if SDL_DEBUG
// must be called only for allocated pages before page_head.reserved is modified
bool page_bpool::valid_checksum(char const * const block_adr, const pageIndex pageId) {
    char const * const page_adr = block_adr + page_head::page_size * page_bit(pageId);
    page_head const * const head = reinterpret_cast<page_head const *>(page_adr);
    if (head->data.pageId.pageId == pageId.value()) {
        if (!head->data.tornBits || head->valid_checksum()) {
            return true;
        }
    }
    SDL_ASSERT(0);
    return false;
}
#endif

page_head const *
page_bpool::lock_page(pageIndex const pageId)
{
    const size_t blockId = pageId.value() / pool_limits::block_page_num;
    SDL_ASSERT(blockId < m_block.size());
    if (!blockId) { // zero block must be always in memory
        SDL_ASSERT(valid_checksum(m_alloc.base(), pageId));
        return zero_block_page(pageId);
    }
    if (info.last_block < blockId) {
        throw_error_t<block_index>("page not found");
        return nullptr;
    }
    lock_guard lock(m_mutex); // should be improved
#if (SDL_DEBUG > 1) && defined(SDL_OS_WIN32)
    if (pageId.value() == 9) {
        free_unused_blocks(); // test only
    }
#endif
    const size_t thread_id = m_thread_id.insert().first;
    SDL_ASSERT(thread_id < pool_limits::max_thread);
    block_index & bi = m_block[blockId];
    if (bi.blockId()) { // block is loaded
        bi.set_lock_page(page_bit(pageId));
        return lock_block_head(bi.blockId(), pageId, thread_id, false);
    }
    else { // block is NOT loaded
        // allocate block or free unused block(s) if not enough space
        if (max_pool_size < info.filesize) {
            SDL_ASSERT(m_alloc.capacity() == max_pool_size);
            if (!m_alloc.can_alloc(pool_limits::block_size)) {
                if (!free_unused_blocks()) {
                    SDL_ASSERT(!"bad alloc");
                }                
            }
        }
        if (char * const block_adr = m_alloc.alloc(pool_limits::block_size)) {
            read_block_from_file(block_adr, blockId);
            SDL_ASSERT(valid_checksum(block_adr, pageId));
            block32 const blockId = m_alloc.block_id(block_adr);
            SDL_ASSERT(m_alloc.get_block(blockId) == block_adr);
            bi.set_blockId(blockId);
            bi.set_lock_page(page_bit(pageId));
            return lock_block_head(blockId, pageId, thread_id, true);
        }
    }
    SDL_ASSERT(0);
    throw_error_t<block_index>("bad alloc");
    return nullptr;
}

bool page_bpool::unlock_page(pageIndex const pageId)
{
    const size_t blockId = pageId.value() / pool_limits::block_page_num;
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
        const size_t pi = page_bit(pageId);
        if (bi.is_lock_page(pi)) {
            if (unlock_block_head(bi.blockId(), pageId, thread_id.first)) {
                if (!bi.clr_lock_page(pi)) {
                    // update used/unused block list...
                    return true;
                }
            }
        }
    }
    return false;
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

bool page_bpool::free_unused_blocks()
{
    // 1st simple approach: scan and sort unused blocks by pageAccessTime
    return false;
#if 0
    const size_t free_target = free_target_size();
    using T = std::pair<block32, uint32>; // pair<id, pageAccessTime>
    std::vector<T> free_block;
    size_t free_size = 0;
    for (block_index & b : m_block) {
        if (b.can_free_unused()) {
            const uint32 t = lastAccessTime(b.blockId());
            SDL_ASSERT(!free_size == free_block.empty());
            if (free_size < free_target) {
                algo::insertion_sort(free_block, T(b.blockId(), t), 
                    [](T const & x, T const & y){
                    return x.second < y.second;
                });
                free_size += pool_limits::block_size;
            }
            else if (free_block.back().second > t) {
                free_block.back() = { b.blockId(), t }; // replace last 
                algo::insertion_sort(free_block.begin(), free_block.end(),
                    [](T const & x, T const & y){
                    return x.second < y.second;
                });
            }
        }
    }
    if (free_size) {
        SDL_ASSERT(!free_block.empty());
        SDL_ASSERT(free_size >= pool_limits::block_size);
        //FIXME: free block chain
    }
    return false;
#endif
}

#if SDL_DEBUG
bool page_bpool::assert_page(pageIndex id)
{
    return lock_page(id) != nullptr;
}
#endif

#if SDL_DEBUG
namespace {
    class unit_test {
        void test();
    public:
        unit_test() {
            if (1) {
                try {
                    test();
                }
                catch(sdl_exception & e) {
                    SDL_TRACE(e.what());
                }
            }
        }
    };
    void unit_test::test() {
        thread_id_t test;
        SDL_ASSERT(test.insert().second);
        SDL_ASSERT(!test.insert().second);
        SDL_ASSERT(test.insert().first < test.size());
        {
            std::vector<std::thread> v;
            for (int n = 0; n < pool_limits::max_thread - 1; ++n) {
                v.emplace_back([&test](){
                    SDL_ASSERT(test.insert().second);
                });
            }
            for (auto& t : v) {
                t.join();
            }
        }
        const auto id = test.get_id();
        SDL_ASSERT(test.find(id).first < test.size());
        SDL_ASSERT(test.erase(id));
        SDL_ASSERT(!test.find(id).second);
        SDL_ASSERT(test.find(id).first == test.size());
    }
    static unit_test s_test;
}
#endif //#if SDL_DEBUG
}}} // sdl
