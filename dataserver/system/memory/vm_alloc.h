// vm_alloc.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_VM_ALLOC_H__
#define __SDL_SYSTEM_MEMORY_VM_ALLOC_H__

#include "dataserver/common/common.h"

namespace sdl { namespace db { namespace mmu {

class vm_alloc : noncopyable {
public:
    explicit vm_alloc(uint64);
    ~vm_alloc();
    uint64 reserved() const;
    void * alloc(uint64 start, uint64 size);
    void clear(uint64 start, uint64 size);
private:
    class data_t;
    const std::unique_ptr<data_t> data;
};

} // mmu
} // db
} // sdl

#endif // __SDL_SYSTEM_MEMORY_VM_ALLOC_H__
