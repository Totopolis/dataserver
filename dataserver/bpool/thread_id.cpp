// thread_id.cpp
//
#include "dataserver/bpool/thread_id.h"
#include "dataserver/common/algorithm.h"

namespace sdl { namespace db { namespace bpool {

thread_mask_t::thread_mask_t(size_t const s)
    : m_filesize(s)
    , m_length(init_length(s))
    , m_head(new node_link)
{
    static_assert(node_megabyte == 8192, "");       // 8 GB per node
    static_assert(node_mask_size == 1024, "");      // 1 bit per megabyte
    static_assert(node_block_num == 131072, "");    // blocks per node
    static_assert(megabyte<1>::value == 16 * pool_limits::block_size, "");
    static_assert(init_length(terabyte<1>::value) == 128, "");
    static_assert(node_t::size() == node_mask_size, "");
    SDL_ASSERT(m_length);
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
        void test();
    public:
        unit_test() {
            if (1) {
                try {
                    test();
                }
                catch(sdl_exception & e) {
                    std::cout << "exceptio = " << e.what() << std::endl;
                }
            }
            if (1) {
                thread_mask_t test(terabyte<1>::value);
            }
        }
    };
    void unit_test::test() {
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
