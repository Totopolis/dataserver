// vm_unix.cpp
//
#include "dataserver/bpool/vm_unix.h"
#include "dataserver/filesys/mmap64_unix.h" // mmap, mmap64

#if defined(SDL_OS_UNIX)
    #if !defined(MAP_ANONYMOUS)
        #define MAP_ANONYMOUS MAP_ANON
    #endif
#endif

#if defined(SDL_OS_UNIX) || SDL_DEBUG
namespace sdl { namespace db { namespace bpool {

#if defined(SDL_OS_UNIX)
char * vm_unix_old::init_vm_alloc(size_t const size, vm_commited const f) {
    if (size && !(size % block_size)) {
#if defined(SDL_OS_UNIX)
        void * const base = mmap64_t::call(nullptr, size, 
            is_commited(f) ? (PROT_READ | PROT_WRITE) : // the desired memory protection of the mapping
                PROT_NONE // pages may not be accessed
            , MAP_PRIVATE | MAP_ANONYMOUS // private copy-on-write mapping. The mapping is not backed by any file
            ,-1 // file descriptor
            , 0 // offset must be a multiple of the page size as returned by sysconf(_SC_PAGE_SIZE)
        );
        throw_error_if_t<vm_unix_old>(!base, "mmap64_t failed");
        return reinterpret_cast<char *>(base);
#endif // SDL_OS_UNIX
    }
    throw_error_t<vm_unix_old>("init_vm_alloc failed");
    return nullptr;
}

vm_unix_old::vm_unix_old(size_t const size, vm_commited const f)
    : byte_reserved(size)
    , page_reserved(size / page_size)
    , block_reserved(size / block_size)
    , m_base_address(init_vm_alloc(size, f))
{
    A_STATIC_ASSERT_64_BIT;
    SDL_ASSERT(size && !(size % block_size));
    SDL_ASSERT(page_reserved <= max_page);
    SDL_ASSERT(block_reserved <= max_block);
    SDL_ASSERT(is_open());
    SDL_DEBUG_CPP(d_block_commit.resize(block_reserved, is_commited(f)));
    SDL_TRACE("vm_unix is_commited = ", is_commited(f));
}

vm_unix_old::~vm_unix_old()
{
    if (m_base_address) {
#if defined(SDL_OS_UNIX)
        if (::munmap(m_base_address, byte_reserved)) {
            SDL_ASSERT(!"munmap");
        }
#endif
    }
}

#if SDL_DEBUG
bool vm_unix_old::assert_address(char const * const start, size_t const size) const {
    SDL_ASSERT(m_base_address <= start);
    SDL_ASSERT(!((start - m_base_address) % block_size));
    SDL_ASSERT(size && !(size % block_size));
    SDL_ASSERT(start + size <= end_address());
    return true;
}

size_t vm_unix_old::count_alloc_block() const {
    return std::count(d_block_commit.begin(), d_block_commit.end(), true);
}
#endif

char * vm_unix_old::alloc(char * const start, const size_t size)
{
    SDL_ASSERT(assert_address(start, size));
#if SDL_DEBUG
    {
        if (0) {
            SDL_TRACE("vm_unix::alloc = ", (start - m_base_address) / block_size);
        }
        size_t b = (start - m_base_address) / block_size;
        const size_t endb = b + (size + block_size - 1) / block_size;
        SDL_ASSERT(b < endb);
        SDL_ASSERT(endb <= block_reserved);
        for (; b < endb; ++b) {
            SDL_ASSERT(!d_block_commit[b]);
            d_block_commit[b] = true;
        }
    }
#endif
#if defined(SDL_OS_UNIX)
    if (::mprotect(start, size, PROT_READ | PROT_WRITE)) {
        SDL_ASSERT(!"mprotect");
        throw_error_t<vm_unix_old>("mprotect PROT_READ|PROT_WRITE failed");
    }
#endif
    return start;
}

// start and size must be aligned to blocks
bool vm_unix_old::release(char * const start, const size_t size)
{
    SDL_ASSERT(assert_address(start, size));
#if SDL_DEBUG
    {
        if (0) {
            SDL_TRACE("vm_unix::release = ", (start - m_base_address) / block_size, " N = ", size / block_size);
        }
        size_t b = (start - m_base_address) / block_size;
        const size_t endb = b + (size + block_size - 1) / block_size;
        SDL_ASSERT(b < endb);
        SDL_ASSERT(endb <= block_reserved);
        for (; b < endb; ++b) {
            SDL_ASSERT(d_block_commit[b]);
            d_block_commit[b] = false;
        }
    }
#endif
#if defined(SDL_OS_UNIX)
    if (::mprotect(start, size, PROT_NONE)) {
        SDL_ASSERT(!"mprotect");
        throw_error_t<vm_unix_old>("mprotect PROT_NONE failed");
    }
#endif
    return true;
}

#endif
//---------------------------------------------------------------

vm_unix_base::vm_unix_base(size_t const size)
    : byte_reserved(size)
    , page_reserved(size / page_size)
    , block_reserved(size / block_size)
    , arena_reserved(size / arena_size)
{
    A_STATIC_ASSERT_64_BIT;
    SDL_ASSERT(size && !(size % arena_size));
}

vm_unix_new::vm_unix_new(size_t const size, vm_commited const f)
    : vm_unix_base(get_arena_size(size) * arena_size)
    , m_arena(arena_reserved)
    , m_free_arena_list{}
    , m_mixed_arena_list{}
{
    A_STATIC_ASSERT_64_BIT;
    SDL_ASSERT(size && !(size % block_size));
    SDL_ASSERT(page_reserved <= max_page);
    SDL_ASSERT(block_reserved <= max_block);
    SDL_ASSERT(byte_reserved == arena_reserved * arena_size);
    A_STATIC_ASSERT_IS_POD(arena_index);
    A_STATIC_ASSERT_IS_POD(block_t);
    A_STATIC_ASSERT_IS_POD(arena_t);
    static_assert(sizeof(arena_index) == 4, "");
    static_assert(sizeof(block_t) == 4, "");
    static_assert(sizeof(arena_t) == 14, "");
    static_assert(block_size * arena_block_num == arena_size, "");
    static_assert(get_arena_size(gigabyte<1>::value) == 1024, "");
    static_assert(get_arena_size(terabyte<1>::value) == 1024*1024, ""); // 1048576
    static_assert(arena_t::mask_all == 0xFFFF, "");
    if (is_commited(f)) {
        for (auto & x : m_arena) {
            alloc_arena(x);
        }
    }
    SDL_ASSERT(!m_free_arena_list);
    SDL_ASSERT(!m_mixed_arena_list);
    SDL_ASSERT(!m_arena_brk);
    SDL_DEBUG_CPP(d_block_commit.resize(block_reserved));
}

vm_unix_new::~vm_unix_new()
{
    for (arena_t & x : m_arena) {
        sys_free_arena(x.arena_adr);
    }
}

size_t vm_unix_new::count_free_arena_list() const {
    size_t result = 0;
    for(auto p = m_free_arena_list; p; ++result) {
        const auto & x = m_arena[p.index()];
        SDL_ASSERT(!x.arena_adr && x.empty());
        p = x.next_arena;
    }
    SDL_ASSERT(result <= m_arena_brk);
    return result;
}

size_t vm_unix_new::count_mixed_arena_list() const {
    size_t result = 0;
    for(auto p = m_mixed_arena_list; p; ++result) {
        const auto & x = m_arena[p.index()];
        SDL_ASSERT(x.arena_adr && x.mixed());
        p = x.next_arena;
    }
    SDL_ASSERT(result <= m_arena_brk);
    return result;
}

char * vm_unix_new::sys_alloc_arena() {
#if defined(SDL_OS_UNIX)
    void * const p = mmap64_t::call(nullptr, arena_size, 
        PROT_READ | PROT_WRITE // the desired memory protection of the mapping
        , MAP_PRIVATE | MAP_ANONYMOUS // private copy-on-write mapping. The mapping is not backed by any file
        ,-1 // file descriptor
        , 0 // offset must be a multiple of the page size as returned by sysconf(_SC_PAGE_SIZE)
    );
    throw_error_if_t<vm_unix_new>(!p, "mmap64_t failed");
    return reinterpret_cast<char *>(p);
#else
    char * const p = reinterpret_cast<char *>(std::malloc(arena_size));
    throw_error_if_t<vm_unix_new>(!p, "bad malloc");
    return p;
#endif
}

bool vm_unix_new::sys_free_arena(char * const p) {
    if (!p)
        return false;
#if defined(SDL_OS_UNIX)
    if (::munmap(p, arena_size)) {
        SDL_ASSERT(!"munmap");
        return false;
    }
#else
    std::free(p);
#endif
    return true;
}

char * vm_unix_new::alloc_block() {
    if (char * const p = alloc_block_without_count()) {
        ++m_alloc_block_count;
        SDL_ASSERT(m_alloc_block_count <= block_reserved);
        SDL_DEBUG_CPP(const block32 b = get_block_id(p));
        SDL_ASSERT(b < block_reserved);
        SDL_ASSERT(!d_block_commit[b]);
        SDL_DEBUG_CPP(d_block_commit[b] = true);
        return p;
    }
    SDL_ASSERT(0);
    return nullptr;
}

bool vm_unix_new::release(char * const p) {
    SDL_DEBUG_CPP(const block32 b = get_block_id(p));
    SDL_ASSERT(b < block_reserved);
    SDL_ASSERT(d_block_commit[b]);
    SDL_DEBUG_CPP(d_block_commit[b] = false);
    if (p && release_without_count(p)) {
        SDL_ASSERT(m_alloc_block_count);
        --m_alloc_block_count;
        return true;
    }
    SDL_ASSERT(0);
    return false;
}

char * vm_unix_new::alloc_next_arena_block() 
{
    SDL_ASSERT(m_arena_brk < arena_reserved);
    if (m_arena_brk == arena_reserved) {
        SDL_ASSERT(0);
        return nullptr;
    }
    const size_t i = m_arena_brk++;
    arena_t & x = m_arena[i];
    alloc_arena(x);
    x.set_block<0>();
    SDL_ASSERT(x.set_block_count() == 1);
    add_to_mixed_arena_list(i);
    return x.arena_adr;
}

char * vm_unix_new::alloc_block_without_count()
{
    SDL_ASSERT(m_arena_brk <= arena_reserved);
    if (!m_arena_brk) {
        SDL_ASSERT(!m_mixed_arena_list);
        SDL_ASSERT(!m_free_arena_list);
        return alloc_next_arena_block();
    }
    if (m_mixed_arena_list) { // use mixed first
        const size_t i = m_mixed_arena_list.index();
        arena_t & x = m_arena[i];
        SDL_ASSERT(x.arena_adr && x.mixed()); 
        const size_t index = x.find_free_block();
        x.set_block(index);
        char * const p = x.arena_adr + (index << power_of<block_size>::value);
        SDL_ASSERT(find_arena(p) == i);
        if (x.full()) {
            m_mixed_arena_list = x.next_arena; // can be null
            x.next_arena.set_null();
        }
        return p;
    }
    if (m_free_arena_list) { // use m_free_arena_list first
        const size_t i = m_free_arena_list.index();
        arena_t & x = m_arena[i];
        SDL_ASSERT(x.empty() && !x.arena_adr);
        m_free_arena_list = x.next_arena; // can be null
        x.next_arena.set_null();
        alloc_arena(x);
        x.set_block<0>();
        SDL_ASSERT(x.set_block_count() == 1);
        add_to_mixed_arena_list(i);
        return x.arena_adr;
    }
    SDL_ASSERT(m_arena_brk);
    SDL_ASSERT(!m_mixed_arena_list);
    SDL_DEBUG_CPP(const arena_t & test = m_arena[m_arena_brk - 1]);
    SDL_ASSERT(test.full() && test.arena_adr);
    return alloc_next_arena_block();
}

void vm_unix_new::add_to_mixed_arena_list(size_t const i)
{
    SDL_ASSERT(!find_mixed_arena_list(i));
    arena_t & x = m_arena[i];
    SDL_ASSERT(!x.next_arena);
    SDL_ASSERT(x.mixed() && x.arena_adr);
    if (m_mixed_arena_list) {
        x.next_arena.set_index(m_mixed_arena_list.index());
    }
    else {
        x.next_arena.set_null();
    }
    m_mixed_arena_list.set_index(i);
}

bool vm_unix_new::remove_from_mixed_arena_list(size_t const i)
{
    if (!m_mixed_arena_list) {
        return false;
    }
    if (m_mixed_arena_list.index() == i) {
        arena_t & x = m_arena[i];
        SDL_ASSERT(x.arena_adr);
        m_mixed_arena_list = x.next_arena;
        x.next_arena.set_null();
        return true;
    }
    arena_index prev = m_mixed_arena_list;
    arena_index p = m_arena[prev.index()].next_arena;
    while (p) {
        SDL_ASSERT(prev);
        arena_t & x = m_arena[p.index()];
        SDL_ASSERT(x.arena_adr);
        if (p.index() == i) {
            m_arena[prev.index()].next_arena = x.next_arena;
            x.next_arena.set_null();
            return true;
        }
        SDL_ASSERT(x.mixed() && x.arena_adr);
        prev = p;
        p = x.next_arena;
    }
    SDL_ASSERT(0);
    return false;
}

bool vm_unix_new::release_without_count(char * const start)
{
    SDL_ASSERT(m_arena_brk && (m_arena_brk <= arena_reserved));
    SDL_ASSERT(start);
    const size_t i = find_arena(start);
    if (i < arena_reserved) {
        arena_t & x = m_arena[i];
        const size_t offset = start - x.arena_adr;
        SDL_ASSERT(!(offset % block_size));
        SDL_ASSERT(offset < arena_size);
        const size_t j = (offset >> power_of<block_size>::value);
        SDL_ASSERT(x.is_block(j));
        x.clr_block(j);
        if (x.empty()) { // release area, add to m_free_area_list
            SDL_ASSERT(!x.block_mask);
            remove_from_mixed_arena_list(i);
            if (m_free_arena_list) {
                x.next_arena.set_index(m_free_arena_list.index());
            }
            else {
                x.next_arena.set_null();
            }
            free_arena(x, i);
            m_free_arena_list.set_index(i);
            SDL_ASSERT(find_free_arena_list(i));
            SDL_ASSERT(!find_mixed_arena_list(i));
            return true;
        }
        SDL_ASSERT(x.mixed());
        if (1 == x.free_block_count()) { // add to m_mixed_arena_list
            SDL_ASSERT(!find_mixed_arena_list(i));
            if (m_mixed_arena_list) {
                x.next_arena.set_index(m_mixed_arena_list.index());
            }
            else {
                x.next_arena.set_null();
            }
            m_mixed_arena_list.set_index(i);
        }
        SDL_ASSERT(find_mixed_arena_list(i));
        return true;
    }
    SDL_ASSERT(0);
    return false;
}

#if SDL_DEBUG
bool vm_unix_new::find_block_in_list(arena_index p, size_t const i) const {
    while (p) {
        if (p.index() == i) {
            return true;
        }
        p = m_arena[p.index()].next_arena;
    }
    return false;
}
bool vm_unix_new::find_free_arena_list(size_t const i) const {
    auto p = m_free_arena_list;
    while (p) {
        if (p.index() == i) {
            SDL_ASSERT(!m_arena[i].arena_adr);
            return true;
        }
        const arena_t & x = m_arena[p.index()];
        SDL_ASSERT(!x.arena_adr);
        p = x.next_arena;
    }
    return false;
}
bool vm_unix_new::find_mixed_arena_list(size_t const i) const {
    auto p = m_mixed_arena_list;
    while (p) {
        if (p.index() == i) {
            SDL_ASSERT(m_arena[i].mixed());
            SDL_ASSERT(m_arena[i].arena_adr);
            return true;
        }
        const arena_t & x = m_arena[p.index()];
        SDL_ASSERT(x.arena_adr && x.mixed());
        p = x.next_arena;
    }
    return false;
}
#endif

size_t vm_unix_new::find_arena(char const * const p) const // not optimized (linear search)
{
    SDL_ASSERT(m_arena_brk && (m_arena_brk <= arena_reserved));
    SDL_ASSERT(p);
    size_t result = 0;
    for (auto const & x : m_arena) {
        SDL_ASSERT(result < m_arena_brk);
        if (x.arena_adr) {
            if ((p >= x.arena_adr) && (p < (x.arena_adr + arena_size))) {
                return result;
            }
        }
        ++result;
    }
    SDL_ASSERT(result == arena_reserved);
    SDL_ASSERT(!"find_arena");
    return result;
}

vm_unix_new::block32
vm_unix_new::get_block_id(char const * const p) const
{
    SDL_ASSERT(p);
    const size_t i = find_arena(p);
    if (i < arena_reserved) {
        const arena_t & x = m_arena[i];
        const size_t offset = p - x.arena_adr;
        SDL_ASSERT(!(offset % block_size));
        SDL_ASSERT(offset < arena_size);
        const size_t j = (offset >> power_of<block_size>::value);
        SDL_ASSERT(x.is_block(j));
        SDL_ASSERT(block_t::init(i, j).value < block_reserved);
        return block_t::init(i, j).value;
    }
    SDL_ASSERT(0);
    return block_index::invalid_block32;
}

char * vm_unix_new::get_block(block32 const id) const
{
    SDL_ASSERT(m_arena_brk && (m_arena_brk <= arena_reserved));
    const block_t b = block_t::init_id(id);
    SDL_ASSERT(b.d.arenaId < arena_reserved);
    SDL_ASSERT(b.d.arenaId < m_arena_brk);
    const arena_t & a = m_arena[b.d.arenaId];
    if (a.arena_adr) {
        if (a.is_block(b.d.index)) {
            static_assert(power_of<block_size>::value == 16, "");
            return a.arena_adr + (size_t(b.d.index) << power_of<block_size>::value);
        }
    }
    SDL_ASSERT(0);
    return nullptr;
}

//---------------------------------------------------------------

#if SDL_DEBUG
namespace {
class unit_test {
    static void test(vm_commited);
public:
    unit_test() {
        test(vm_commited::false_);
        test(vm_commited::true_);
        SDL_TRACE_FUNCTION;
    }
};

void unit_test::test(vm_commited const flag) {
    if (1) {
        using T = vm_unix_new;
        T test(T::arena_size * 2 + T::block_size * 3, flag);
        for (size_t j = 0; j < 2; ++j) {
            for (size_t i = 0; i < test.block_reserved; ++i) {
                if (char * const p = test.alloc_block()) {
                    const auto b = test.get_block_id(p);
                    SDL_ASSERT(b < test.block_reserved);
                    SDL_ASSERT(p == test.get_block(b));
                    if (0 == j) {
                        SDL_ASSERT(test.release(p));
                    }
                }
            }
        }
    }
    if (1) {
        using T = vm_unix_new;
        enum { test_size = T::arena_size * 2 + T::block_size * 3 };
        T test(test_size, flag);
        SDL_ASSERT(test.byte_reserved >= test_size);
        std::vector<char *> block_adr;
        for (size_t i = 0; i < (test_size / T::block_size); ++i) {
            block_adr.push_back(test.alloc_block());
            const size_t t2 = test.count_mixed_arena_list();
            if ((i + 1) % T::arena_block_num) {
                SDL_ASSERT(t2 == 1);
            }
            else {
                SDL_ASSERT(t2 == 0);
            }
        }
        {
            const size_t t0 = test.arena_brk();
            const size_t t1 = test.count_free_arena_list();
            const size_t t2 = test.count_mixed_arena_list();
            SDL_ASSERT(t0 == 3);
            SDL_ASSERT(t1 == 0);
            SDL_ASSERT(t2 == 1);
        }
        for (size_t i = 0; i < (test.arena_block_num * 2 + 1); ++i) {
            if (test.release(block_adr[i])) {
                block_adr[i] = nullptr;
            }
        }
        {
            const size_t t0 = test.arena_brk();
            const size_t t1 = test.count_free_arena_list();
            const size_t t2 = test.count_mixed_arena_list();
            SDL_ASSERT(t0 == 3);
            SDL_ASSERT(t1 == 2);
            SDL_ASSERT(t2 == 1);
        }
        //FIXME: random test
    }
}

static unit_test s_test;
}
#endif // SDL_DEBUG
}}} // db
#endif // SDL_OS_UNIX
