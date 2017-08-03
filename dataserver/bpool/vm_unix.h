// vm_unix.h
//
#pragma once
#ifndef __SDL_BPOOL_VM_UNIX_H__
#define __SDL_BPOOL_VM_UNIX_H__

#if defined(SDL_OS_UNIX) || SDL_DEBUG

#include "dataserver/bpool/vm_base.h"
#include "dataserver/bpool/block_head.h"

namespace sdl { namespace db { namespace bpool { 

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

class vm_unix_new final : public vm_base {
    using block32 = block_index::block32;
public:
    enum { arena_size = megabyte<1>::value }; // 1 MB = 2^20 = 1048,576
#pragma pack(push, 1) 
    struct arena_t {
        static constexpr uint16 mask_all = uint16(-1); // 0xFFFF
        char * arena_adr;
        uint16 block_mask;
        void zero_arena();
        template<size_t> void set_block();
        template<size_t> bool is_block() const;
        void set_block(size_t);
        bool is_block(size_t) const;
        bool is_full() const;
    };
    struct block_t { // 4 bytes
        static constexpr size_t max_arena = (1 << 20);
        static constexpr size_t max_index = (1 << 4);
        struct data_type {
            unsigned int arenaId : 20;  // 1 terabyte address space (2^20 * 1MB)
            unsigned int index : 4;     // block index in arena (16 blocks = 16 * 64 KB = 1MB)
#if SDL_DEBUG
            unsigned int zero : 8;   // zero pad
#else
            unsigned int : 8;   // zero pad
#endif
        };
        union {
            data_type d;
            uint32 value;
        };
        static block_t init_id(const uint32 value) {
            block_t b;
            b.value = value;
            SDL_ASSERT(!b.d.zero);
            return b;
        }
        static block_t init(const size_t arenaId, const size_t index) {
            SDL_ASSERT(arenaId < max_arena);
            SDL_ASSERT(index < max_index);
            block_t b {};
            b.d.arenaId = arenaId;
            b.d.index = index;
            return b;
        }
    };
#pragma pack(pop)
    static constexpr size_t get_arena_size(const size_t size) {
        return round_up_div(size, (size_t)arena_size);
    }
public:
    size_t const byte_reserved;
    size_t const page_reserved;
    size_t const block_reserved;
    size_t const arena_reserved;
public:
    explicit vm_unix_new(size_t, vm_commited);
    ~vm_unix_new();
    char * alloc_block();
    bool release(char *);
    block32 get_block_id(char const *) const; // block must be allocated
    char * get_block(block32) const; // block must be allocated
private:
#if SDL_DEBUG
    static bool debug_zero_arena(arena_t & x) {
        x.zero_arena();
        return true;
    }
#endif
    char * alloc_arena();
    void free_arena(char *);
    size_t find_arena(char const *) const;
    char * alloc_next_arena_block();
private:
    using vector_arena_t = std::vector<arena_t>;
    vector_arena_t m_arena;
    size_t m_arena_brk = 0;
    char * m_free_block_list = nullptr;
};

inline void vm_unix_new::arena_t::zero_arena(){
    SDL_ASSERT(arena_adr);
    memset(arena_adr, 0, arena_size);
}
inline bool vm_unix_new::arena_t::is_block(size_t const i) const {
    SDL_ASSERT(arena_adr);
    SDL_ASSERT(i < 16);
    return 0 != (block_mask & (uint16)(1 << i));
}
template<size_t i>
inline bool vm_unix_new::arena_t::is_block() const {
    SDL_ASSERT(arena_adr);
    static_assert(i < 16, "");
    return 0 != (block_mask & (uint16)(1 << i));
}
inline void vm_unix_new::arena_t::set_block(size_t const i) {
    SDL_ASSERT(arena_adr);
    SDL_ASSERT(i < 16);
    SDL_ASSERT(!is_block(i));
    block_mask |= (uint16)(1 << i);
}
template<size_t i>
void vm_unix_new::arena_t::set_block() {
    SDL_ASSERT(arena_adr);
    static_assert(i < 16, "");
    SDL_ASSERT(!is_block<i>());
    block_mask |= (uint16)(1 << i);
}
inline bool vm_unix_new::arena_t::is_full() const {
    SDL_ASSERT(arena_adr);
    return block_mask == arena_t::mask_all;
}

}}} // db

#endif // SDL_OS_UNIX
#endif // __SDL_BPOOL_VM_UNIX_H__
