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

thread_id_t::thread_id_t(size_t const s)
    : init_thread_id(std::this_thread::get_id())
    , m_filesize(s)
    , m_size(0)
{
    SDL_ASSERT(m_filesize);
    SDL_ASSERT(!empty(init_thread_id));
}

thread_id_t::pos_mask
thread_id_t::find(thread_id const id) {
    SDL_ASSERT(id != init_thread_id);
    SDL_ASSERT(!empty(id));
    const auto pos = std::find_if(m_data.begin(), m_data.end(), [id](id_mask const & x){
        SDL_ASSERT(empty(x.first) == !x.second);
        return (x.first == id);
    });
    if (pos != m_data.end()) {
        SDL_ASSERT(pos->second);
        const size_t d = static_cast<size_t>(std::distance(m_data.begin(), pos));
        return { d, pos->second.get() };
    }
    return{ max_thread, nullptr };
}

thread_id_t::pos_mask
thread_id_t::insert(thread_id const id) {
    SDL_ASSERT(id != init_thread_id);
    SDL_ASSERT(!empty(id));
    auto pos = std::find_if(m_data.begin(), m_data.end(), [id](id_mask const & x) {
        return (x.first == id) || (x.second == nullptr);
    });
    if (pos == m_data.end()) {
        SDL_ASSERT(0); //FIXME: wait until thread is released ?
        throw_error_t<thread_id_t>("too many threads");
        return { max_thread, nullptr };
    }
    if (pos->first != id) {
        SDL_ASSERT(empty(pos->first)); // empty slot is found
        SDL_ASSERT(!pos->second);
        const auto pos_id = std::find_if(pos + 1, m_data.end(), [id](id_mask const & x) {
            return (x.first == id);
        });
        if (pos_id == m_data.end()) { // use empty slot
            pos->first = id;
            reset_new(pos->second, m_filesize);
            SDL_ASSERT(m_size < (int)max_size());
            ++m_size;
            SDL_TRACE("thread_insert ", id, ", m_size ", m_size);
        }
        else {
            pos = pos_id;
        }
    }
    SDL_ASSERT(pos->second);
    const auto d = static_cast<size_t>(std::distance(m_data.begin(), pos));
    return { d, pos->second.get() };
}

bool thread_id_t::erase(thread_id const id) {
    SDL_ASSERT(id != init_thread_id);
    SDL_ASSERT(!empty(id));
    const auto pos = std::find_if(m_data.begin(), m_data.end(), [id](id_mask const & x){
        return (x.first == id);
    });
    if (pos != m_data.end()) {
        SDL_ASSERT(pos->second);
        *pos = {};
        SDL_ASSERT(m_size);
        --m_size;
        SDL_TRACE("* thread_erase ", id, ", m_size ", m_size);
        return true;
    }
    SDL_ASSERT(!"erase");
    return false;
}

#if SDL_DEBUG
namespace {
    inline constexpr uint32_t knuth_hash(uint32_t v) {
        return (v * UINT32_C(2654435761)) % thread_id_t::max_size();
    }
    class unit_test {
        void test_thread();
        void test_mask(size_t);
    public:
        unit_test() {
            static_assert(power_of<64>::value == 6, "");
            if (0) {
                test_mask(gigabyte<8>::value);
                //test_mask(terabyte<1>::value);
            }
            if (0) {
                try {
                    test_thread();
                }
                catch(sdl_exception & e) {
                    std::cout << "exception = " << e.what() << std::endl;
                }
            }
            if (0) {
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
