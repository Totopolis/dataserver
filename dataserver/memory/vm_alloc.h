// vm_alloc.h
//
#pragma once
#ifndef __SDL_MEMORY_VM_ALLOC_H__
#define __SDL_MEMORY_VM_ALLOC_H__

#include "dataserver/common/common.h"

namespace sdl { namespace db { namespace mmu {

class vm_alloc : noncopyable {
public:
    explicit vm_alloc(uint64);
    ~vm_alloc();
    uint64 byte_reserved() const;
    uint64 page_reserved() const;
    void * alloc(uint64 start, uint64 size);
    bool clear(uint64 start, uint64 size);
private:
    class data_t;
    const std::unique_ptr<data_t> data;
};

} // mmu
} // db
} // sdl

#endif // __SDL_MEMORY_VM_ALLOC_H__
