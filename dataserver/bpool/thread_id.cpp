// thread_id.cpp
//
#include "dataserver/bpool/thread_id.h"

namespace sdl { namespace db { namespace bpool { 

thread_mask_t::thread_mask_t(size_t const filesize)
    : m_index_count(round_up_div(filesize, index_size))
    , m_block_count(round_up_div(filesize, (size_t)block_size))
{
    SDL_ASSERT(m_index_count <= max_index);
    SDL_ASSERT(m_block_count <= pool_limits::max_block);
    static_assert(index_block_num == 8192, "");
    static_assert(!(index_block_num % mask_div), "");
    static_assert(sizeof(mask_t) == 1024, "");
    m_index.resize(m_index_count);
}

void thread_mask_t::shrink_to_fit() {
    for (mask_p & p : m_index) {
        if (p && empty(*p)) {
            p.reset();
        }
    }
}

void thread_mask_t::clear() {
    for (mask_p & p : m_index) {
        p.reset();
    }
}

//-------------------------------------------------------------
#if SDL_DEBUG
namespace {
    static bool is_unit_test = 0;
}
#endif

thread_id_t::thread_id_t(size_t const s)
    : m_filesize(s)
#if SDL_DEBUG
    , init_thread_id(get_id())
#endif
{
    static_assert(data_type::size() == max_thread, "");
}

thread_id_t::pos_mask
thread_id_t::find(id_type const id) {
    if (use_hash) {
        const size_t h = hash_id(id);
        id_mask const & x = m_data[h];
        if ((x.first == id) && (x.second != nullptr)) {
            return { h, x.second.get() };
        }
    }
    const auto pos = std::find_if(m_data.begin(), m_data.end(),
        [id](id_mask const & x){
        return (x.first == id) && (x.second != nullptr);
    });
    if (pos != m_data.end()) {
        const size_t d = std::distance(m_data.begin(), pos);
        return { d, pos->second.get() };
    }
    return{ max_thread, nullptr };
}

thread_id_t::pos_mask
thread_id_t::insert(id_type const id) {
    if (use_hash) {
        const size_t h = hash_id(id);
        id_mask & x = m_data[h];
        if ((x.first == id) || (x.second == nullptr)) {
            if (!x.second) { // empty slot is found
                x.first = id;
                reset_new(x.second, m_filesize);
            }
            return { h, x.second.get() };
        }
    }
    const auto pos = std::find_if(m_data.begin(), m_data.end(),
        [id](id_mask const & x){
        return (x.first == id) || (x.second == nullptr);
    });
    if (pos == m_data.end()) {
        throw_error_t<thread_id_t>("too many threads");
        return { max_thread, nullptr };
    }
    if (!pos->second) { // empty slot is found
        pos->first = id;
        reset_new(pos->second, m_filesize);
    }
    const size_t d = std::distance(m_data.begin(), pos);
    return{ d, pos->second.get() };
}

bool thread_id_t::erase(id_type const id) {
    if (use_hash) {
        const size_t h = hash_id(id);
        id_mask & x = m_data[h];
        if ((x.first == id) && (x.second != nullptr)) {
            x = {};
            return true;
        }
    }
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
            static_assert(power_of<64>::value == 6, "");
            is_unit_test = true;
            SDL_UTILITY_SCOPE_EXIT([](){
                is_unit_test = false;
            })
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
        SDL_ASSERT(pos.first.value() == test.insert().first.value());
        const auto id = test.get_id();
        SDL_ASSERT(test.find(id).first.value() < pool_limits::max_thread);
        SDL_ASSERT(test.find(id).second);
        SDL_ASSERT(test.erase(id));
        SDL_ASSERT(!test.erase(id));
        SDL_ASSERT(!test.find(id).second);
        SDL_TRACE_FUNCTION;
    }
    static unit_test s_test;
}
#endif //#if SDL_DEBUG
}}} // sdl
