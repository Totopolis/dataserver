// thread_id.cpp
//
#include "dataserver/bpool/thread_id.h"
#include "dataserver/common/algorithm.h"

namespace sdl { namespace db { namespace bpool { 

thread_mask_t::thread_mask_t(size_t const filesize)
    : m_filesize(filesize)
    , m_index_count(utils::round_up_div<index_size>(filesize))
    , m_block_count(utils::round_up_div<block_size>(filesize))
{
    SDL_ASSERT(m_index_count <= max_index);
    SDL_ASSERT(m_block_count <= pool_limits::max_block);
    static_assert(index_block_num == 8192, "");
    static_assert(!(index_block_num % mask_div), "");
    m_index.resize(m_index_count);
}

bool thread_mask_t::is_block(size_t const i) const {
    SDL_ASSERT(i < m_block_count);
    mask_p const & p = m_index[i / index_block_num];
    if (p) {
        const size_t j = (i % index_block_num) / mask_div;
        const size_t k = (i & mask_hex);
        uint64 const & mask = (*p)[j];
        return (mask & (uint64(1) << k)) != 0;
    }
    return false;
}

void thread_mask_t::clr_block(size_t const i) {
    SDL_ASSERT(i < m_block_count);
    mask_p & p = m_index[i / index_block_num];
    if (p) {
        const size_t j = (i % index_block_num) / mask_div;
        const size_t k = (i & mask_hex);
        uint64 & mask = (*p)[j];
        mask &= ~(uint64(1) << k);
    }
}

void thread_mask_t::set_block(size_t const i) {
    SDL_ASSERT(i < m_block_count);
    mask_p & p = m_index[i / index_block_num];
    if (!p) {
        reset_new(p);
    }
    const size_t j = (i % index_block_num) / mask_div;
    const size_t k = (i & mask_hex);
    uint64 & mask = (*p)[j];
    mask |= (uint64(1) << k);
}

inline bool thread_mask_t::empty(mask_t const & m) const {
    for (const auto & v : m) {
        if (v)
            return false;
    }
    return true;
}

void thread_mask_t::shrink_to_fit() {
    for (auto & p : m_index) {
        if (p && empty(*p)) {
            p.reset();
        }
    }
}

//-------------------------------------------------------------

thread_id_t::size_bool
thread_id_t::insert(id_type const id)
{
    const size_bool ret = algo::unique_insertion_distance(m_data, id);
    if (ret.second) {
        if (size() > max_thread) {
            SDL_ASSERT(!"max_thread");
            this->erase_id(id);
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

bool thread_id_t::erase_id(id_type const id) {
    return algo::binary_erase(m_data, id);
}

void thread_id_t::erase_pos(size_t const i) {
    m_data.erase(m_data.begin() + i);
}


#if SDL_DEBUG
namespace {
    class unit_test {
        void test_thread();
        void test_mask(size_t);
    public:
        unit_test() {
            if (1) {
                test_mask(gigabyte<8>::value);
                //test_mask(terabyte<1>::value);
            }
            if (1) {
                try {
                    test_thread();
                }
                catch(sdl_exception & e) {
                    std::cout << "exception = " << e.what() << std::endl;
                }
            }
        }
    };
    void unit_test::test_mask(size_t const filesize) {
        thread_mask_t test(filesize);
        for (size_t i = 0; i < test.size(); ++i) {
            SDL_ASSERT(!test[i]);
            test.set_block(i);
            SDL_ASSERT(test[i]);
            if (i >= 8192)
                test.clr_block(i);
        }
        test.clr_block(test.size()-1);
        test.shrink_to_fit();
    }
    void unit_test::test_thread() {
        thread_id_t test(gigabyte<8>::value);
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
        SDL_ASSERT(test.erase_id(id));
        SDL_ASSERT(!test.find(id).second);
        SDL_ASSERT(test.find(id).first == test.size());
        while (!test.empty()) {
            test.erase_pos(0);
        }
        SDL_TRACE_FUNCTION;
    }
    static unit_test s_test;
}
#endif //#if SDL_DEBUG
}}} // sdl
