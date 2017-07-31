// vm_base.h
//
#pragma once
#ifndef __SDL_BPOOL_VM_BASE_H__
#define __SDL_BPOOL_VM_BASE_H__

#include "dataserver/system/page_head.h"

namespace sdl { namespace db { namespace bpool {

class vm_base : noncopyable {
public:
    enum { block_page_num = 8 };
    enum { page_size = page_head::page_size };                      // 8 KB = 8192 byte = 2^13
    enum { block_size = page_size * block_page_num };               // 64 KB = 65536 byte = 2^16
    static constexpr size_t max_page = size_t(1) << 32;             // 4,294,967,296 = 2^32
    static constexpr size_t max_block = max_page / block_page_num;  // 536,870,912 = 2^29
};

enum class vm_commited { false_, true_ };

inline constexpr bool is_commited(vm_commited v) {
    return v != vm_commited::false_;
}

}}} // db

#endif // __SDL_BPOOL_VM_BASE_H__
