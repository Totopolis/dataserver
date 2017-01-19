// stdcommon.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#ifndef __SDL_COMMON_STDCOMMON_H__
#define __SDL_COMMON_STDCOMMON_H__

#if defined(__clang__)
#if (__cplusplus == 201402L)
    //https://llvm.org/bugs/show_bug.cgi?id=18402
    #ifndef _GLIBCXX_HAVE_GETS
        extern "C" char* gets (char* __s) __attribute__((deprecated));
    #endif
#endif
#endif

#include <cstdint>
#include <stdio.h>
#include <cstdio> // for snprintf 
#include <cstring> // for memset
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <utility>
#include <array>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <cmath> // std::fabs, std::atan2
#include <cstdlib> // std::mbstowcs, atof
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <assert.h>

#endif // __SDL_COMMON_STDCOMMON_H__