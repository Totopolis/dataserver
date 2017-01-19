// stdcommon.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#ifndef __SDL_COMMON_STDCOMMON_H__
#define __SDL_COMMON_STDCOMMON_H__

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

#if defined(__clang__) && (__cplusplus == 201402L)
// reproduced on Clang 3.5.0, C++14, Linux
// usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/c++/4.8/cstdio:120:11: error: 
// no member named 'gets' in the global namespace
// using ::gets;
//-----------------------------------
// Hack around stdlib bug with C++14.
#include <initializer_list>  // force libstdc++ to include its config
#undef _GLIBCXX_HAVE_GETS    // correct broken config
// End hack.
//-----------------------------------
#endif

#endif // __SDL_COMMON_STDCOMMON_H__