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
SDL_DEBUG_HPP(public:)
    enum { arena_size = megabyte<1>::value }; // 1 MB = 2^20 = 1048,576
#pragma pack(push, 1) 
    struct arena_t {
        char * arena_adr;
        uint8 block_mask;
        void set_zero();
    };
    struct block_t { // 4 bytes
        struct data_type {
            unsigned int arenaId : 20;  // 1 terabyte address space (2^20 * 1MB)
            unsigned int offset : 4;    // block index in arena (16 blocks = 16 * 64 KB = 1MB)
            unsigned int : 8;           // zero pad
        };
        union {
            data_type d;
            uint32 value;
        };
    };
#pragma pack(pop)
    static constexpr size_t get_arena_size(const size_t size) {
        return round_up_div(size, (size_t)arena_size);
    }
public:
    size_t const byte_reserved;
    size_t const page_reserved;
    size_t const block_reserved;
public:
    explicit vm_unix_new(size_t, vm_commited);
    ~vm_unix_new();
    char * alloc(char * start, size_t);
    bool release(char * start, size_t);
    block32 get_block_id(char const *) const; // block must be allocated
    char * get_block(block32) const; // block must be allocated
private:
    char * alloc_arena();
    void free_arena(char *);
private:
    std::vector<arena_t> m_arena;
    char * m_free_block_list = nullptr;
};

inline void vm_unix_new::arena_t::set_zero(){
    SDL_ASSERT(arena_adr);
    if (arena_adr) {
        memset(arena_adr, 0, arena_size);
    }
    block_mask = 0;
}

}}} // db

#endif // SDL_OS_UNIX
#endif // __SDL_BPOOL_VM_UNIX_H__
