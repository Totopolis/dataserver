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

size_t thread_id_t::find(id_type const id) const
{
    return std::distance(m_data.begin(), algo::binary_find(m_data, id));
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
    , m_alloc(base_page_bpool::max_pool_size, vm_commited::false_) // will be improved
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
    SDL_ASSERT(!m_alloc_brk);
    char * const block_adr = m_alloc.alloc(m_alloc.base_address(), pool_limits::block_size);
    if (block_adr) {
        SDL_ASSERT(block_adr == m_alloc.base_address());
        m_alloc_brk = block_adr + pool_limits::block_size;
        SDL_ASSERT(m_alloc_brk > m_alloc.base_address());
        SDL_ASSERT(m_alloc_brk <= m_alloc.end_address());
        m_file.read(block_adr, 0, info.block_size_in_bytes(0));
        m_block[0].set_lock_page_all();
    }
    else {
        throw_error_t<page_bpool>("bad alloc");
    }
}

void page_bpool::update_block_head(page_head * const p, const size_t thread_id)
{
    SDL_ASSERT(thread_id < pool_limits::max_thread);
    block_head * const head = get_block_head(p);
    head->set_lock_thread(thread_id);
    head->pageAccessTime = ++m_accessCnt;
    // update used/unused block list
}

#if SDL_DEBUG
// must be called only for allocated pages before page_head.reserved is modified
bool page_bpool::valid_checksum(char const * const block_adr, const pageIndex pageId) {
    char const * const page_adr = block_adr + page_head::page_size * page_offset(pageId);
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
    lock_guard lock(m_mutex); // should be improved
    const size_t blockId = pageId.value() / pool_limits::block_page_num;
    SDL_ASSERT(blockId < m_block.size());
    if (!blockId) { // zero block must be always in memory
        SDL_ASSERT(valid_checksum(m_alloc.base_address(), pageId));
        return zero_block_page(pageId);
    }
    if (info.last_block < blockId) {
        throw_error_t<block_index>("page not found");
        return nullptr;
    }
    const size_t thread_id = m_thread_id.insert().first;
    SDL_ASSERT(thread_id < pool_limits::max_thread);
    block_index & bi = m_block[blockId];
    if (bi.offset()) { // block is loaded
        const size_t pi = page_offset(pageId);
        bi.set_lock_page(pi);
        char * const block_adr = m_alloc.base_address() + bi.offset();
        char * const page_adr = block_adr + pi * pool_limits::page_size;
        page_head * const p = reinterpret_cast<page_head *>(page_adr);
        update_block_head(p, thread_id);
        SDL_ASSERT(p->data.pageId.pageId == pageId.value());
        return p;
    }
    else { // block is NOT loaded
        // allocate block or free unused block(s) if not enough space
        SDL_ASSERT(m_alloc_brk >= m_alloc.base_address());
        SDL_ASSERT(m_alloc_brk <= m_alloc.end_address());
        if (0) {
            const size_t used_memory_size = m_alloc_brk - m_alloc.base_address();
            if (max_pool_size < info.filesize) {
                if (used_memory_size + pool_limits::block_size > max_pool_size) {
                    //FIXME: free unused block
                    SDL_ASSERT(0);
                }
            }
        }
        if (m_alloc_brk < m_alloc.end_address()) {
            char * const block_adr = m_alloc.alloc(m_alloc_brk, pool_limits::block_size);
            if (block_adr) {
                m_alloc_brk += pool_limits::block_size;
                SDL_ASSERT(m_alloc_brk <= m_alloc.end_address());
                m_file.read(block_adr, blockId * pool_limits::block_size, 
                    info.block_size_in_bytes(blockId)); // read block from file
                SDL_ASSERT(valid_checksum(block_adr, pageId));
                const size_t offset = block_adr - m_alloc.base_address(); 
                SDL_ASSERT(offset);
                bi.set_offset(offset); // init block_index
                const size_t pi = page_offset(pageId);
                bi.set_lock_page(pi);
                char * const page_adr = block_adr + pi * pool_limits::page_size;
                page_head * const p = reinterpret_cast<page_head *>(page_adr);
                update_block_head(p, thread_id);
                SDL_ASSERT(p->data.pageId.pageId == pageId.value());
                return p;
            }
        }
    }
    SDL_ASSERT(0);
    throw_error_t<block_index>("bad alloc");
    return nullptr;
}

bool page_bpool::unlock_page(pageIndex const pageId)
{
    return false;
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
        SDL_ASSERT(test.find(id) < test.size());
        SDL_ASSERT(test.erase(id));
        SDL_ASSERT(test.find(id) == test.size());
    }
    static unit_test s_test;
}
#endif //#if SDL_DEBUG
}}} // sdl
