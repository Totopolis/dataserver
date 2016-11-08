// outstream.cpp
//
#include "dataserver/common/common.h"
#include "dataserver/common/outstream.h"

namespace sdl {

file_utils::unique_ofstream
file_utils::open_file(std::string const & out_file) 
{
    if (out_file.empty()) {
        SDL_ASSERT(0);
        return{};
    }
    auto outfile = sdl::make_unique<std::ofstream>(out_file, std::ofstream::out|std::ofstream::trunc);
    if (outfile->rdstate() & std::ifstream::failbit) {
        outfile.reset();
        throw_error<sdl_exception_t<file_utils>>("error opening file");
    }
    return outfile;
}

} //namespace sdl
