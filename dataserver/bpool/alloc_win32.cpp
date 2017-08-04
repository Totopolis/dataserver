// alloc_win32.cpp
//
#include "dataserver/bpool/alloc_win32.h"

#if defined(SDL_OS_WIN32)

namespace sdl { namespace db { namespace bpool {

page_bpool_alloc_win32::page_bpool_alloc_win32(const size_t size)
    : m_alloc(get_alloc_size(size), vm_commited::false_)
{
    m_alloc_brk = m_alloc.base_address();
    throw_error_if_t<page_bpool_alloc_win32>(!m_alloc_brk, "bad alloc");
    SDL_ASSERT(size <= capacity());
    SDL_TRACE(__FUNCTION__, " size = ", size, " ", size / megabyte<1>::value, " MB");
}

#if SDL_DEBUG
bool page_bpool_alloc_win32::assert_brk() const {
    SDL_ASSERT(m_alloc_brk >= m_alloc.base_address());
    SDL_ASSERT(m_alloc_brk <= m_alloc.end_address());
    return true;
}
#endif

char * page_bpool_alloc_win32::alloc_block() {
    if (block_size <= unused_size()) {
        if (m_decommit) { // use decommitted blocks first 
            if (char * const result = m_alloc.alloc(get_block(m_decommit.front()), block_size)) {
                m_decommit.pop_front();
                return result;
            }
        }
        else if (auto result = m_alloc.alloc(m_alloc_brk, block_size)) {
            SDL_ASSERT(result == m_alloc_brk);
            m_alloc_brk += block_size;
            SDL_ASSERT(assert_brk());
            return result;
        }
    }
    SDL_ASSERT(!"bad alloc");
    return nullptr;
}

bool page_bpool_alloc_win32::release(block_list_t & free_block_list)
{
    if (!free_block_list) {
        return false; // normal case
    }
    SDL_DEBUG_CPP(auto const test_length = free_block_list.length());
    SDL_DEBUG_CPP(const size_t count_alloc_block1 = m_alloc.count_alloc_block());
    interval_block32 decommit;
    free_block_list.for_each([this, &decommit](block_head const * const p, block32 const id){
        SDL_ASSERT(get_block(id) == (char *)block_head::get_page_head(p));
        if (!decommit.insert(id)) { 
            SDL_ASSERT(0);
        }
    }, freelist::true_);
    SDL_ASSERT(decommit.size() == test_length);
    const auto done = decommit.for_each2([this](block32 const x, block32 const y){
        SDL_ASSERT(x <= y);
        const size_t size = static_cast<size_t>(y - x + 1) * block_size;
        static_assert((int)block_size == (int)vm_base::block_size, "");
        if (!m_alloc.release(get_block(x), size)) {
            SDL_ASSERT(0);
            return false;
        }
        return true;
    });
    if (is_break(done)) {
        SDL_ASSERT(0);
        return false;
    }
    SDL_ASSERT(count_alloc_block1 == m_alloc.count_alloc_block() + test_length);
    m_decommit.merge(std::move(decommit));
    SDL_ASSERT(m_decommit && !decommit);
    free_block_list.clear();
    { // adjust m_alloc_brk
        const auto last = m_decommit.back2(); 
        char const * const lask_decommit = get_block(last.second) + block_size;
        SDL_ASSERT(lask_decommit <= m_alloc_brk);
        SDL_ASSERT(last.first <= last.second);
        if (lask_decommit == m_alloc_brk) {
            m_alloc_brk = get_block(last.first);
            m_decommit.pop_back2();
        }
    }
    SDL_ASSERT(can_alloc(test_length * block_size));
    SDL_ASSERT(used_size() + unused_size() == capacity());
    return true;
}

#if SDL_DEBUG
namespace {
class unit_test {
public:
    unit_test() {
        if (1) {
            enum { N = 8 };
            page_bpool_alloc_win32 test(pool_limits::block_size * N);
            SDL_ASSERT(!test.used_block());
            SDL_ASSERT(test.unused_block() == N);
            for (size_t i = 0; i < N; ++i) {
                SDL_ASSERT(test.alloc_block());
            }
            SDL_ASSERT(0 == test.unused_size());
            SDL_ASSERT(!test.can_alloc(pool_limits::block_size));
            SDL_ASSERT(test.used_block() == N);
            SDL_ASSERT(!test.unused_block());
        }
    }
};
static unit_test s_test;
}
#endif // SDL_DEBUG
}}} // sdl

#endif // #if defined(SDL_OS_WIN32)