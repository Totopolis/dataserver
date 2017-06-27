// space.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_SPACE_H__
#define __SDL_SYSTEM_MEMORY_SPACE_H__

#include "dataserver/system/page_head.h"

namespace sdl { namespace db { namespace vm {

// virtual memory : reserve region, allocate blocks, 8192 byte blocks (custom heap)
// page address translation
// two-level paging?
// relocate/optimize address space using spatial cell_id (hilbert curve)
// 8-page segment (extent), 8192 * 8 = 65536 byte space = segment bitmask
// 65536 / 8 =  8192 segments, 8192 / 8 = 1024 byte segment bitmask
// page address translation (2 bytes per page id, 65536 * 8192 = 536,870,912 byte address space)
// hybrid memory model : virtual memory (fast) + mmf (other data, os handles)
// memory usage statistics
// 1-2 level indexx (b-tree) in fast memory
// separate system (indexx, schema) and user (data) memory
// pfs_for_page : only allocated pages loaded/mapped to memory
// optionally, manual call to free/compact memory
// pinned/fixed and movable pages, LRU cache policy or clock algorithm, clock or access counter
// https://en.wikipedia.org/wiki/Page_replacement_algorithm
// https://en.wikipedia.org/wiki/Cache_replacement_policies
// https://en.wikipedia.org/wiki/Adaptive_replacement_cache
// https://en.wikipedia.org/wiki/Translation_lookaside_buffer
// https://en.wikipedia.org/wiki/Heap_(data_structure)

} // vm
} // db
} // sdl

#endif // __SDL_SYSTEM_MEMORY_SPACE_H__