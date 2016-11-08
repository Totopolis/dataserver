// generator_util.cpp
//
#include "dataserver/maketable/generator_util.h"
#include <algorithm>

namespace sdl { namespace db { namespace make {

std::string & util::replace(std::string & s, const char * const token, const std::string & value) {
    size_t const n = strlen(token);
    size_t pos = 0;
    while (1) {
        const size_t i = s.find(token, pos, n);
        if (i != std::string::npos) {
            s.replace(i, n, value);
            pos = i + n;
        }
        else
            break;
    }
    SDL_ASSERT(pos);
    return s;
}

std::string util::remove_extension( std::string const& filename ) {
    auto pivot = std::find(filename.rbegin(), filename.rend(), '.');
    if (pivot == filename.rend()) {
        return filename;
    }
    return std::string(filename.begin(), pivot.base() - 1);
}

std::string util::extract_filename(const std::string & path, const bool remove_ext) {
    if (!path.empty()) {
        std::string basename(std::find_if( path.rbegin(), path.rend(), [](char ch) {
            return ch == '\\' || ch == '/'; 
        }).base(), path.end());
        return remove_ext ? remove_extension(basename) : basename;
    }
    return{};
}

util::vector_string
util::split(const std::string & s, char const token) {
    SDL_ASSERT(token);
    if (!s.empty()) {
        vector_string result;
        size_t p1 = 0;
        size_t p2 = s.find(token);
        while (p2 != std::string::npos) {
            if (p2 > p1) {
                result.push_back(s.substr(p1, p2 - p1));
            }
            p2 = s.find(token, p1 = p2 + 1);
        }
        if (p1 < s.size()) {
            result.push_back(s.substr(p1));
        }
        return result;
    }
    return {};
}

bool util::is_find(vector_string const & vec, const std::string & name) {
    SDL_ASSERT(!name.empty());
    for (auto const & s : vec) {
        SDL_ASSERT(!s.empty());
        if (s == name) 
            return true;
    }
    return false;
}

} // make
} // db
} // sdl

#if SDL_DEBUG
namespace sdl {
    namespace db {
        namespace {
            class unit_test {
            public:
                unit_test()
                {
                    SDL_ASSERT(make::util::split("").empty());
                    auto const t1 = make::util::split("1 2 3");
                    auto const t2 = make::util::split(" 1 2 3 ");
                    SDL_ASSERT(make::util::is_find(t2, "2"));
                    SDL_ASSERT(t1 == t2);
                    SDL_ASSERT(t1.size() == 3);
                    SDL_ASSERT(make::util::split("123").size() == 1);
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG