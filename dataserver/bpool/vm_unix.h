// vm_unix.h
//
#pragma once
#ifndef __SDL_BPOOL_VM_UNIX_H__
#define __SDL_BPOOL_VM_UNIX_H__

#include "dataserver/bpool/vm_base.h"
#include "dataserver/bpool/block_head.h"

namespace sdl { namespace db { namespace bpool { 

#if 0 // defined(SDL_OS_UNIX)
class vm_unix_old final : public vm_base {
public:
    size_t const byte_reserved;
    size_t const page_reserved;
    size_t const block_reserved;
public:
    explicit vm_unix_old(size_t, vm_commited);
    ~vm_unix_old();
    char * base_address() const {
        return m_base_address;
    }
    char * end_address() const {
        return m_base_address + byte_reserved;
    }
    bool is_open() const {
        return m_base_address != nullptr;
    }
    char * alloc(char * start, size_t);
    bool release(char * start, size_t);
#if SDL_DEBUG
    size_t count_alloc_block() const;
#endif
private:
#if SDL_DEBUG
    bool assert_address(char const *, size_t) const;
#endif
    static char * init_vm_alloc(size_t, vm_commited);
private:
    char * const m_base_address = nullptr;
    SDL_DEBUG_HPP(std::vector<bool> d_block_commit;)
};
#endif

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
        return round_up_div(filesize, (size_t)arena_size);
    }
    explicit vm_unix_base(size_t);
};

class vm_unix_new final : public vm_unix_base {
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
        void clr_block(size_t);
        void set_block(size_t);
        bool is_block(size_t) const;
        bool full() const;
        bool empty() const;
        bool mixed() const;
        size_t used_block_count() const;
        size_t free_block_count() const;
        size_t find_free_block() const;
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
            block_t b;
            b.value = value;
            SDL_ASSERT(!b.d.zeros);
            return b;
        }
        static block_t init(const size_t arenaId, const size_t index) {
            SDL_ASSERT(arenaId < max_arena);
            SDL_ASSERT(index < max_index);
            block_t b {};
            b.d.index = index;
            b.d.arenaId = arenaId;
            SDL_ASSERT(!b.d.zeros);
            return b;
        }
    };
#pragma pack(pop)
public:
    vm_unix_new(size_t, vm_commited);
    ~vm_unix_new();
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
    interval_block32 defragment(interval_block32 const &);
private:
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
    void alloc_arena(arena_t &, size_t);
    void free_arena(arena_t &, size_t);
    size_t find_arena(char const *) const;
    char * alloc_next_arena_block();
    void add_to_free_arena_list(arena_t &, size_t);
    void add_to_mixed_arena_list(arena_t &, size_t);
    bool remove_from_mixed_arena_list(size_t);
    using vector_arena32 = std::vector<arena32>;
    vector_arena32 copy_mixed_arena_list() const;
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

inline bool vm_unix_new::release_block(block32 const id) {
    return release(get_block(id));
}

}}} // db

#include "dataserver/bpool/vm_unix.inl"

#endif // __SDL_BPOOL_VM_UNIX_H__
