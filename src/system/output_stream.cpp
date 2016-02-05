// output_stream.cpp :
//
#include "common/common.h"
#include "output_stream.h"

namespace sdl {

scoped_redirect::scoped_redirect(ostream & _original, ostream & _redirect)
    : original(_original)
    , redirect(_redirect)
{
    original.rdbuf(redirect.rdbuf(original.rdbuf()));
}

scoped_redirect::~scoped_redirect()
{
    original.rdbuf(redirect.rdbuf(original.rdbuf()));
}

} // sdl