// file_map_unix.cpp
//
#if defined(SDL_OS_UNIX)

#include "common/common.h"
#include "file_map_detail.h"
#include "file_h.h"
#include <sys/mman.h>
#include <utility>

namespace sdl { namespace {

struct substitution_failure {}; // represent a failure to declare something

template<typename T> 
struct substitution_succeeded : std::true_type {};
        
template<>
struct substitution_succeeded<substitution_failure> : std::false_type {};

struct get_mmap64 {
private:
    template<typename X>
    static auto check(X const& x) -> decltype(mmap64(x, 0, 0, 0, 0, 0));
    static substitution_failure check(...);
public:
    using type = decltype(check(nullptr));
};

struct has_mmap64: substitution_succeeded<get_mmap64::type> {};

template<bool>
struct select_mmap64 {
    template<typename... Values>
    static void * get(Values&&... params) {
        return mmap(std::forward<Values>(params)...);
    }
};

template<>
struct select_mmap64<true> {
    template<typename... Values>
    static void * get(Values&&... params) {
        return mmap64(std::forward<Values>(params)...);
    }
};

} // namespace

file_map_detail::view_of_file 
file_map_detail::map_view_of_file(const char* filename,
                                  uint64 const offset,  
                                  uint64 const size)
{
    A_STATIC_ASSERT_64_BIT; 

    SDL_ASSERT(size);
    SDL_ASSERT(!offset); // current limitation
    SDL_ASSERT(offset < size);

    if (size && (offset < size) && !offset) {

        FileHandler fp(filename, "rb");
        if (!fp.is_open()) {
            SDL_TRACE_2("FileHandler failed to open file: ", filename);
            SDL_ASSERT(false);
            return nullptr;
        }
        auto pFileView = select_mmap64<has_mmap64::value>::get(
            nullptr, static_cast<size_t>(size), 
            PROT_READ, MAP_PRIVATE, fileno(fp.get()), 0);

        if (pFileView == MAP_FAILED) {
            SDL_TRACE_2("mmap failed: ", filename);
            SDL_ASSERT(false);
            return nullptr;
        }
        static_assert(std::is_same<view_of_file, decltype(pFileView)>::value, "");
        return pFileView;
    }
    return nullptr;
}

bool file_map_detail::unmap_view_of_file(
    view_of_file p,
    uint64 const offset,
    uint64 const size)
{
    if (p) {
        SDL_ASSERT(offset < size);
        ::munmap(p, size);
        return true;
    }
    SDL_ASSERT(!offset);
    SDL_ASSERT(!size);
    return false;
}

} // sdl

#if SDL_DEBUG
namespace sdl { namespace {

class unit_test {
public:
    unit_test()
    {
        SDL_TRACE_FILE;
        //SDL_TRACE_2("has_mmap64::value = ", has_mmap64::value);
    }
};
static unit_test s_test;

} // namespace
} // sdl
#endif //#if SV_DEBUG
#endif //#if defined(SDL_OS_UNIX)