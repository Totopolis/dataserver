// spatial_tree.h
//
#pragma once
#ifndef __SDL_SYSTEM_SPATIAL_TREE_H__
#define __SDL_SYSTEM_SPATIAL_TREE_H__

#include "spatial_tree_t.h"

namespace sdl { namespace db { 

using spatial_tree = spatial_tree_t<int64>;
using shared_spatial_tree = std::shared_ptr<spatial_tree>;

} // db
} // sdl

#endif // __SDL_SYSTEM_SPATIAL_TREE_H__
