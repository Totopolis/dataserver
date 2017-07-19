// thread_id.cpp
//
#include "dataserver/bpool/thread_id.h"
#include "dataserver/common/algorithm.h"

namespace sdl { namespace db { namespace bpool {

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
