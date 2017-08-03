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

}}} // db

#endif // SDL_OS_UNIX
#endif // __SDL_BPOOL_VM_UNIX_H__
