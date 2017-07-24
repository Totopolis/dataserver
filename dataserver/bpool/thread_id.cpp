// thread_id.cpp
//
#include "dataserver/bpool/thread_id.h"

namespace sdl { namespace db { namespace bpool { 

thread_mask_t::thread_mask_t(size_t const filesize)
    : m_index_count(utils::round_up_div<index_size>(filesize))
    , m_block_count(utils::round_up_div<block_size>(filesize))
{
    SDL_ASSERT(m_index_count <= max_index);
    SDL_ASSERT(m_block_count <= pool_limits::max_block);
    static_assert(index_block_num == 8192, "");
    static_assert(!(index_block_num % mask_div), "");
    static_assert(sizeof(mask_t) == 1024, "");
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

thread_id_t::thread_id_t(size_t const s)
    : m_filesize(s)
{
    static_assert(data_type::size() == max_thread, "");
}

thread_id_t::size_bool
thread_id_t::find(id_type const id) const {
    const auto pos = std::find_if(m_data.begin(), m_data.end(),
        [id](id_mask const & x){
        return (x.first == id) && (x.second != nullptr);
    });
    const size_t d = std::distance(m_data.begin(), pos);
    SDL_ASSERT(d <= max_thread);
    return { d, pos != m_data.end() };
}

size_t thread_id_t::insert(id_type const id) {
    const auto pos = std::find_if(m_data.begin(), m_data.end(),
        [id](id_mask const & x){
        return (x.first == id) || (x.second == nullptr);
    });
    if (pos == m_data.end()) {
        SDL_ASSERT(0);
        throw_error_t<thread_id_t>("too many threads");
        return max_thread;
    }
    if (!pos->second) { // empty slot is found
        pos->first = id;
        reset_new(pos->second, m_filesize);
    }
    return std::distance(m_data.begin(), pos);
}

bool thread_id_t::erase(id_type const id) {
    const auto pos = std::find_if(m_data.begin(), m_data.end(),
        [id](id_mask const & x){
        return (x.first == id) && (x.second != nullptr);
    });
    if (pos != m_data.end()) {
        *pos = {};
        return true;
    }
    return false;
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
        auto pos = test.insert();
        SDL_ASSERT(pos == test.insert());
        {
            std::vector<std::thread> v;
            for (int n = 0; n < pool_limits::max_thread - 1; ++n) {
                v.emplace_back([&test](){
                    test.insert();
                });
            }
            for (auto& t : v) {
                t.join();
            }
        }
        const auto id = test.get_id();
        SDL_ASSERT(test.find(id).first < pool_limits::max_thread);
        SDL_ASSERT(test.erase(id));
        SDL_ASSERT(!test.erase(id));
        SDL_ASSERT(!test.find(id).second);
        SDL_TRACE_FUNCTION;
    }
    static unit_test s_test;
}
#endif //#if SDL_DEBUG
}}} // sdl
