// mem_utils.cpp
//
#include "dataserver/system/mem_utils.h"

namespace sdl { namespace db {

nchar_t const * reverse_find(
    nchar_t const * const begin,
    nchar_t const * const end,
    nchar_t const * const buf,
    size_t const buf_size)
{
    SDL_ASSERT(buf_size);
    SDL_ASSERT(begin <= end);
    const size_t n = end - begin;
    if (n >= buf_size) {
        nchar_t const * p = end - buf_size;
        for (; begin <= p; --p) {
            if (!::memcmp(p, buf, sizeof(buf[0]) * buf_size)) {
                return p;
            }
        }
    }
    return nullptr;
}

mem_utils::vector_char
mem_utils::make_vector(vector_mem_range_t const & array) { //FIXME: will be replaced : iterate memory without copy
    SDL_ASSERT(!array.empty());
    if (array.size() == 1) {
        return { array[0].first, array[0].second };
    }
    else {
        vector_char data;
        data.reserve(mem_size(array));
        for (auto const & m : array) {
            SDL_ASSERT(!mem_empty(m)); // warning
            data.insert(data.end(), m.first, m.second);
        }
        SDL_ASSERT(data.size() == mem_size(array));
        return data;
    }
}

mem_utils::vector_char
mem_utils::make_vector_n(vector_mem_range_t const & array, const size_t size) {
    if (mem_size(array) < size) {
        SDL_ASSERT(0);
        return {};
    }
    if (array.size() == 1) {
        SDL_ASSERT(mem_size(array[0]) >= size);
        const char * const first = array[0].first;
        return { first, first + size };
    }
    else {
        vector_char data;
        data.reserve(size);
        size_t count = size;
        for (auto const & m : array) {
            SDL_ASSERT(!mem_empty(m)); // warning
            const size_t n = a_min(count, mem_size(m));
            data.insert(data.end(), m.first, m.first + n);
            count -= n;
            if (!count) break;
        }
        SDL_ASSERT(data.size() == size);
        return data;
    }
}

bool mem_utils::memcpy_n(void * const dest, vector_mem_range_t const & src, const size_t size)
{
    SDL_ASSERT(dest);
    if (mem_size(src) < size) {
        SDL_ASSERT(0);
        memset(dest, 0, size);
        return false;
    }
    if (src.size() == 1) {
        SDL_ASSERT(mem_size(src[0]) >= size);
        memcpy(dest, src[0].first, size);
        return true;
    }
    else {
        size_t count = size;
        char * buf = reinterpret_cast<char *>(dest);
        for (auto const & m : src) {
            SDL_ASSERT(!mem_empty(m)); // warning
            const size_t n = a_min(count, mem_size(m));
            memcpy(buf, m.first, n);
            count -= n;
            if (!count) break;
            buf += n;
        }
        SDL_ASSERT(!count);    
        return true;
    }
}

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
                    SDL_TRACE_FILE;
                    SDL_ASSERT(IS_LITTLE_ENDIAN);
                    static_assert(sizeof(uint8) == 1, "");
                    static_assert(sizeof(int16) == 2, "");
                    static_assert(sizeof(uint16) == 2, "");
                    static_assert(sizeof(int32) == 4, "");
                    static_assert(sizeof(uint32) == 4, "");
                    static_assert(sizeof(int64) == 8, "");
                    static_assert(sizeof(uint64) == 8, "");
                    static_assert(sizeof(nchar_t) == 2, "");
                    static_assert(is_power_two(2), "");

                    A_STATIC_ASSERT_IS_POD(nchar_t);
                    static_assert(sizeof(nchar_t) == 2, "");
                    {
                        const nchar_t test1[4] = { nchar(0x006F), nchar(0x006E), nchar(0x0030), nchar(0x0030) };
                        const nchar_t test2[4] = { nchar(0x0074), nchar(0x0069), nchar(0x006F), nchar(0x006E) };
                        const nchar_t nzero[2] = { nchar(0x0030), nchar(0x0030) };
                        SDL_ASSERT(reverse_find({ test1, test1 + count_of(test1) }, nzero) == test1 + 2);
                        SDL_ASSERT(reverse_find({ test2, test2 + count_of(test2) }, nzero) == nullptr);
                    }
                    {
                        const std::vector<char> d0(5, '1');
                        const std::vector<char> d1(10, '2');
                        vector_mem_range_t array(2, {});
                        array[0] = mem_range_t(d0.data(), d0.data() + d0.size());
                        array[1] = mem_range_t(d1.data(), d1.data() + d1.size());
                        SDL_ASSERT(mem_size(array) == d0.size() + d1.size());
                        auto const v = mem_utils::make_vector(array);
                        SDL_ASSERT(v.size() == mem_size(array));
                    }
                    {
                        A_STATIC_ASSERT_IS_POD(first_second<double, int>);
                    }
                }
            };
            static unit_test s_test;
        }
    } // db
} // sdl
#endif //#if SV_DEBUG

#if 0
/*
 *----------------------------------------------------------------------
 *
 * strstr --
 *
 *  Locate the first instance of a substring in a string.
 *
 * Results:
 *  If string contains substring, the return value is the
 *  location of the first matching instance of substring
 *  in string.  If string doesn't contain substring, the
 *  return value is 0.  Matching is done on an exact
 *  character-for-character basis with no wildcards or special
 *  characters.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
char *
strstr(string, substring)
    register char *string;  /* String to search. */
    char *substring;        /* Substring to try to find in string. */
{
    register char *a, *b;

    /* First scan quickly through the two strings looking for a
     * single-character match.  When it's found, then compare the
     * rest of the substring.
     */

    b = substring;
    if (*b == 0) {
        return string;
    }
    for ( ; *string != 0; string += 1) {
        if (*string != *b) {
            continue;
        }
        a = string;
        while (1) {
            if (*b == 0) {
                return string;
            }
            if (*a++ != *b++) {
                break;
            }
        }
        b = substring;
    }
    return (char *) 0;
}
#endif
