// thread_id.cpp
//
#include "dataserver/bpool/thread_id.h"
#include "dataserver/common/algorithm.h"

namespace sdl { namespace db { namespace bpool { 

thread_mask_t::thread_mask_t(size_t const s)
    : filesize(s)
    , max_megabyte(utils::round_up_div<megabyte<1>::value>(s))
    , m_length(utils::round_up_div<gigabyte<node_gigabyte>::value>(s))
    , m_head(new node_link)
{
    static_assert(node_megabyte == 8192, "");       // 8 GB per node
    static_assert(node_mask_size == 1024, "");      // 1 bit per megabyte
    static_assert(node_block_num == 131072, "");    // blocks per node
    static_assert(megabyte<1>::value == 16 * pool_limits::block_size, "");
    static_assert(node_t::size() == node_mask_size, "");
    static_assert(utils::round_up_div<gigabyte<node_gigabyte>::value>(terabyte<1>::value) == 128, "max length");
    SDL_ASSERT(m_length);
    SDL_ASSERT(max_megabyte * megabyte<1>::value <= m_length * gigabyte<node_gigabyte>::value);
    SDL_ASSERT(m_head->first.size() == node_mask_size, "");
    node_link * p = m_head.get();
    for (size_t i = 1; i < m_length; ++i) {
        SDL_ASSERT(!p->second);
        reset_new(p->second);
        p = p->second.get();
    }
    SDL_ASSERT(!p->second);
}

thread_mask_t::node_link const *
thread_mask_t::find(size_t i) const
{
    SDL_ASSERT(i < m_length);
    node_link const * p = m_head.get();
    while (i--) {
        p = p->second.get();
    }
    SDL_ASSERT(p);
    return p;
}

thread_mask_t::node_link *
thread_mask_t::find(size_t i)
{
    SDL_ASSERT(i < m_length);
    node_link * p = m_head.get();
    while (i--) {
        p = p->second.get();
    }
    SDL_ASSERT(p);
    return p;
}

bool thread_mask_t::is_megabyte(size_t const i) const {
    SDL_ASSERT(i < max_megabyte);
    node_link const * const p = find(i / node_megabyte);
    size_t const j = i % node_megabyte; // bit index
    size_t const k = j / 8; // byte index
    SDL_ASSERT(k < node_mask_size);
    uint8 const & mask = (p->first)[k];
    return mask & (1 << uint8(j & 7));
}

void thread_mask_t::clr_megabyte(size_t const i) {
    SDL_ASSERT(i < max_megabyte);
    node_link * const p = find(i / node_megabyte);
    size_t const j = i % node_megabyte; // bit index
    size_t const k = j / 8; // byte index
    SDL_ASSERT(k < node_mask_size);
    uint8 & mask = (p->first)[k];
    mask &= ~(1 << uint8(j & 7));
}

void thread_mask_t::set_megabyte(size_t const i) {
    SDL_ASSERT(i < max_megabyte);
    node_link * const p = find(i / node_megabyte);
    size_t const j = i % node_megabyte; // bit index
    size_t const k = j / 8; // byte index
    SDL_ASSERT(k < node_mask_size);
    uint8 & mask = (p->first)[k];
    mask |= (1 << uint8(j & 7));
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
                test_mask(gigabyte<10>::value);
                //test_mask(terabyte<1>::value);
            }
            if (1) {
                try {
                    test_thread();
                }
                catch(sdl_exception & e) {
                    std::cout << "exceptio = " << e.what() << std::endl;
                }
            }
        }
    };
    void unit_test::test_mask(size_t const filesize) {
        thread_mask_t test(filesize);
        for (size_t i = 0; i < test.size(); ++i) {
            SDL_ASSERT(!test[i]);
            test.set_megabyte(i);
            SDL_ASSERT(test[i]);
            test.clr_megabyte(i);
            SDL_ASSERT(!test[i]);
        }
        test.clr_megabyte(test.size()-1);
        SDL_TRACE_FUNCTION;
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
