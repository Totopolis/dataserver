// vm_unix.h
//
#pragma once
#ifndef __SDL_BPOOL_VM_UNIX_H__
#define __SDL_BPOOL_VM_UNIX_H__

#if defined(SDL_OS_UNIX) || SDL_DEBUG

#include "dataserver/bpool/vm_base.h"

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
#if SDL_DEBUG
public:
#endif
    enum { arena_size = megabyte<1>::value }; // 1 MB = 2^20 = 1048,576
#pragma pack(push, 1) 
    struct arena_t {
        char * arena_adr;
        uint8 block_mask;
    };
    struct block_t {
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
public:
    size_t const byte_reserved;
    size_t const page_reserved;
    size_t const block_reserved;
public:
    explicit vm_unix_new(size_t, vm_commited);
    ~vm_unix_new();
    char * alloc(char * start, size_t);
    bool release(char * start, size_t);
private:
    static constexpr size_t get_arena_size(const size_t size) {
        return round_up_div(size, (size_t)arena_size);
    }
    std::vector<arena_t> m_arena;
};


}}} // db

#endif // SDL_OS_UNIX
#endif // __SDL_BPOOL_VM_UNIX_H__
