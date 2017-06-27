// space.h
//
#pragma once
#ifndef __SDL_SYSTEM_MEMORY_SPACE_H__
#define __SDL_SYSTEM_MEMORY_SPACE_H__

#include "dataserver/system/page_head.h"

namespace sdl { namespace db { namespace ms {

// virtual memory : reserve region, allocate blocks, 8192 byte blocks (custom heap)
// page address translation
// move/optimize address space using spatial cell_id (hilbert curve)
// 32 page segment (8192 * 32 = 262144) : segment allocated or empty
// page address translation (2 bytes per page id, 65536 * 8192 = 536,870,912 bytes address space)
// hybrid memory model : virtual memory (fast) + mmf (other data, os handles)
// memory usage statistics
// 1-2 level indexx (b-tree) in fast memory
// separate system and tables memory
// pfs_for_page : only allocated pages loaded/mapped to memory
// optionally, manual call to free/compact memory
// pinned/fixed and movable pages, LRU cache policy or clock algorithm
// https://en.wikipedia.org/wiki/Page_replacement_algorithm
// https://en.wikipedia.org/wiki/Cache_replacement_policies
// https://docs.microsoft.com/en-us/dotnet/standard/garbage-collection/fundamentals

} // ms
} // db
} // sdl

#endif // __SDL_SYSTEM_MEMORY_SPACE_H__
