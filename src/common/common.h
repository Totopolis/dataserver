// common.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef __SDL_COMMON_COMMON_H__
#define __SDL_COMMON_COMMON_H__

#pragma once

#include <cstdint>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <utility>
#include <exception>
#include <assert.h>

#include "config.h"
#include "static.h"
#include "noncopyable.h"
#include "quantity.h"

namespace sdl {

class sdl_exception : public std::exception
{
    using base_type = std::exception;
public:
    explicit sdl_exception(const char* what_arg): base_type(what_arg) {}
};

} // sdl

#endif // __SDL_COMMON_COMMON_H__