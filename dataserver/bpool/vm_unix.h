// vm_unix.h
//
#pragma once
#ifndef __SDL_BPOOL_VM_UNIX_H__
#define __SDL_BPOOL_VM_UNIX_H__

#include "dataserver/bpool/vm_base.h"
#include "dataserver/bpool/block_head.h"

namespace sdl { namespace db { namespace bpool { 

class vm_unix_base : public vm_base {
public:
    enum { arena_size = megabyte<1>::value }; // 1 MB = 2^20 = 1048,576
    enum { arena_block_num = 16 };
    size_t const byte_reserved;
    size_t const page_reserved;
    size_t const block_reserved;
    size_t const arena_reserved;
protected:
    static constexpr size_t get_arena_size(const size_t filesize) {
        static_assert(arena_size == block_size * arena_block_num, "");
        return round_up_div(filesize, (size_t)arena_size);
    }
    explicit vm_unix_base(size_t);
};

class vm_unix final : public vm_unix_base {
    using block32 = uint32;
    using arena32 = uint32;
public:
#pragma pack(push, 1) 
    class arena_index { // 4 bytes
        arena32 value;
    public:
        size_t index() const {
            SDL_ASSERT(value);
            return value - 1;
        }
        void set_index(size_t const v) {
            value = static_cast<arena32>(v) + 1;
        }
        bool is_null() const {
            return !value;
        }
        explicit operator bool() const {
            return !is_null();
        }
        void set_null() { // = {}
            value = 0;
        }
    };
    struct arena_t { // 14 bytes
        using mask16 = uint16;
        static constexpr mask16 mask_all = uint16(-1); // 0xFFFF
        char * arena_adr;          // address of allocated area
        arena_index next_arena;    // index of free_area_list or m_mixed_arena_list (starting from 1) 
        mask16 block_mask;
        void zero_arena();
        template<size_t> void set_block();
        template<size_t> bool is_block() const;
        static void clr_block(mask16 &, size_t);
        void clr_block(size_t);
        void set_block(size_t);
        static bool is_block(mask16, size_t);
        bool is_block(size_t) const;
        bool full() const;
        bool empty() const;
        bool mixed() const;
        size_t set_block_count() const; // <= arena_block_num
        size_t free_block_count() const;
        size_t find_free_block() const;
        size_t find_set_block() const;
        static size_t find_free_block(mask16);
        static size_t find_set_block(mask16);
    };
    struct block_t { // 4 bytes
        static constexpr size_t max_arena = (1 << 20);
        static constexpr size_t max_index = (1 << 4);
        struct data_type {
            unsigned int index : 4;     // block index in arena (16 blocks = 16 * 64 KB = 1MB)
            unsigned int arenaId : 20;  // 1 terabyte address space (2^20 * 1MB)
            unsigned int zeros : 8;     // zero pad
        };
        union {
            data_type d;
            uint32 value;
        };
        static block_t init_id(const uint32 value) {
            static_assert(sizeof(data_type) == sizeof(uint32), "");
            block_t b; // uninitialized
            b.value = value;
            SDL_ASSERT(!b.d.zeros);
            return b;
        }
        static block_t init(const size_t arenaId, const size_t index) {
            SDL_ASSERT(arenaId < block_t::max_arena);
            SDL_ASSERT(index < block_t::max_index);
            static_assert(max_index == arena_block_num, "");
            block_t b {};
            b.d.index = index;
            b.d.arenaId = arenaId;
            SDL_ASSERT(!b.d.zeros);
            return b;
        }
    };
#pragma pack(pop)
public:
    explicit vm_unix(size_t, vm_commited);
    ~vm_unix();
    char * alloc_block();
    bool release(char *);
    bool release_block(block32);
    block32 get_block_id(char const *) const; // block must be allocated
    char * get_block(block32) const; // block must be allocated
    size_t used_size() const {
        SDL_ASSERT(m_alloc_block_count <= block_reserved);
        SDL_ASSERT((m_alloc_block_count * block_size) <= byte_reserved);
        return m_alloc_block_count * block_size;
    }
    size_t commited_size() const {
        return m_alloc_arena_count * arena_size;
    }
    size_t count_free_arena_list() const;
    size_t count_mixed_arena_list() const;
    size_t arena_brk() const {
        return m_arena_brk;
    }
    size_t alloc_block_count() const {
        return m_alloc_block_count;
    }
    size_t alloc_arena_count() const {
        return m_alloc_arena_count;
    }
    using move_block_fun = std::function<bool(block32 from, block32 to)>;
    bool defragment(move_block_fun const &);
private:
    char * get_free_block(block_t const &) const; // block NOT allocated
    char * alloc_block_without_count();
    bool release_without_count(char *);
#if SDL_DEBUG
    static bool debug_zero_arena(arena_t & x) {
        x.zero_arena();
        return true;
    }
    bool find_block_in_list(arena_index, size_t) const;
    bool find_free_arena_list(size_t) const;
    bool find_mixed_arena_list(size_t) const;
#endif
    char * sys_alloc_arena();
    bool sys_free_arena(char *);
    void alloc_arena_nosort(arena_t &, size_t);
    void alloc_arena(arena_t &, size_t);
    void free_arena(arena_t &, size_t);
    size_t find_arena(char const *) const;
    char * alloc_next_arena_block();
    void add_to_free_arena_list(arena_t &, size_t);
    void add_to_mixed_arena_list(arena_t &, size_t);
    bool remove_from_mixed_arena_list(size_t);
private:
    using vector_arena_t = std::vector<arena_t>;
    vector_arena_t m_arena;
    size_t m_arena_brk = 0;
    arena_index m_free_arena_list; // list of released arena(s)
    arena_index m_mixed_arena_list; // list of arena(s) with allocated and free block(s)
    size_t m_alloc_block_count = 0;
    size_t m_alloc_arena_count = 0;
private:
    enum { use_sort_arena = 1 };
    using sort_adr_t = std::vector<arena32>;
    sort_adr_t m_sort_adr; // arena(s) index sorted by arena_adr
    void sort_adr();
    sort_adr_t::iterator find_sort_adr(arena32);
};

inline bool vm_unix::release_block(block32 const id) {
    return release(get_block(id));
}

}}} // db

#include "dataserver/bpool/vm_unix.inl"

#endif // __SDL_BPOOL_VM_UNIX_H__
