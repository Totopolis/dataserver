// page_bpool.cpp
//
#include "dataserver/bpool/page_bpool.h"

namespace sdl { namespace db { namespace bpool {

pool_info_t::pool_info_t(const size_t s)
    : filesize(s)
    , page_count(s / page_size)
    , block_count((s + block_size - 1) / block_size)
{
    SDL_ASSERT(filesize > block_size);
    SDL_ASSERT(!(filesize % page_size));
    static_assert(is_power_two(block_page_num), "");
    const size_t n = page_count % block_page_num;
    last_block = block_count - 1;
    last_block_page_count = n ? n : block_page_num;
    last_block_size = page_size * last_block_page_count;
    SDL_ASSERT((last_block_size >= page_size) && (last_block_size <= block_size));
}

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
    SDL_ASSERT(!"valid_filesize");
    return false;
}

//------------------------------------------------------

thread_id_t::size_bool
thread_id_t::insert(id_type const id)
{
    if (sortorder) {
        return algo::unique_insertion_distance(m_data, id);
    }
    size_t i = 0;
    for (const auto & p : m_data) { // optimize of speed
        if (p == id) {
            return { i, false };
        }
        ++i;
    }
    SDL_ASSERT(m_data.size() == i);
    m_data.push_back(id);
    return { i, true };
}

size_t thread_id_t::find(id_type const id) const
{
    if (sortorder) {
        return std::distance(m_data.begin(), algo::binary_find(m_data, id));
    }
    return std::distance(m_data.begin(), algo::find(m_data, id));
}

bool thread_id_t::erase(id_type const id)
{
    if (sortorder) {
        return algo::binary_erase(m_data, id);
    }
    return algo::erase(m_data, id);
}

threadIndex thread_tlb_t::insert() {
    const auto pos = thread_id.insert();
    if (0 && pos.second) { // temporal disable
        thread_mk.emplace_back(new mask_type(bi.byte_size));
        SDL_ASSERT(thread_mk.back()->size() == bi.byte_size);
    }
    SDL_ASSERT(pos.first < pool_limits::max_thread);
    return pos.first;
}

//------------------------------------------------------

page_bpool::page_bpool(const std::string & fname,
                       const size_t min_size,
                       const size_t max_size)
    : base_page_bpool(fname)
    , min_pool_size(min_size ? a_min(min_size, m_file.filesize()) : min_size)
    , max_pool_size(max_size ? a_min(max_size, m_file.filesize()) : max_size)
    , m_block(info)
    , m_tlb(info)
{
    SDL_TRACE_FUNCTION;
    SDL_ASSERT(min_size <= max_size);
    SDL_ASSERT(min_pool_size <= max_pool_size);
    SDL_ASSERT(m_file.is_open());
}

page_bpool::~page_bpool()
{
}

bool page_bpool::is_open() const
{
    return false;
}

void const * page_bpool::start_address() const
{
    return nullptr;
}

size_t page_bpool::page_count() const
{
    return info.page_count;
}

page_head const *
page_bpool::lock_page(pageIndex const pageId)
{
    const threadIndex ti = m_tlb.insert();
    block_index & b = m_block.find(pageId);
    if (b.d.index) { // block is loaded
    }
    else { // load block
    }
    b.set_page(pageId.value() % 8);
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
    public:
        unit_test() {
            if (1) {
                thread_id_t test;
                SDL_ASSERT(test.insert().second);
                SDL_ASSERT(!test.insert().second);
                SDL_ASSERT(test.insert().first < test.size());
                {
                    std::vector<std::thread> v;
                    for (int n = 0; n < 10; ++n) {
                        v.emplace_back([&test](){
                            SDL_ASSERT(test.insert().second);
                        });
                    }
                    for (auto& t : v) {
                        t.join();
                    }
                    joinable_thread run([&test](){
                        SDL_ASSERT(test.insert().second);
                    });
                }
                SDL_ASSERT(test.size() == 12);
                const auto id = test.get_id();
                SDL_ASSERT(test.find(id) < test.size());
                SDL_ASSERT(test.erase(id));
                SDL_ASSERT(test.find(id) == test.size());
            }
            if (1) {
                using T = pool_limits;
                const pool_info_t info(gigabyte<5>::value + T::page_size);
                thread_tlb_t test(info);
            }
        }
    };
    static unit_test s_test;
}
#endif //#if SDL_DEBUG
}}} // sdl
