// vm_alloc.h
//
#pragma once
#ifndef __SDL_BPOOL_VM_ALLOC_H__
#define __SDL_BPOOL_VM_ALLOC_H__

#if defined(SDL_OS_WIN32)
#include "dataserver/bpool/vm_win32.h"
#else
#include "dataserver/bpool/vm_unix.h"
#endif

namespace sdl { namespace db { namespace bpool {

#if defined(SDL_OS_WIN32)
using vm_alloc = vm_win32;
#else
using vm_alloc = vm_unix;
#endif
using unique_vm_alloc = std::unique_ptr<vm_alloc>;

}}} // sdl

#endif // __SDL_BPOOL_VM_ALLOC_H__
