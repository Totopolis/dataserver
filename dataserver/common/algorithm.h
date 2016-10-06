// algorithm.h
//
#pragma once
#ifndef __SDL_COMMON_ALGORITHM_H__
#define __SDL_COMMON_ALGORITHM_H__

#include <algorithm>

namespace sdl { namespace algo {

template<class T, class key_type>
inline bool is_find(T const & result, key_type const & value) {
    return std::find(result.begin(), result.end(), value) != result.end();
}

template<class T, class key_type>
bool binary_insertion(T & result, key_type && unique_key) {
    if (!result.empty()) {
        const auto pos = std::lower_bound(result.begin(), result.end(), unique_key);
        if (pos != result.end()) {
            if (*pos == unique_key) {
                return false;
            }
            result.insert(pos, std::forward<key_type>(unique_key));
            return true;
        }
    }
    result.push_back(std::forward<key_type>(unique_key)); 
    return true;
}

template<class T, class key_type, class fun_type>
bool binary_insertion(T & result, key_type && unique_key, fun_type compare) {
    if (!result.empty()) {
        const auto pos = std::lower_bound(result.begin(), result.end(), unique_key, compare);
        if (pos != result.end()) {
            if (*pos == unique_key) {
                SDL_ASSERT(!compare(*pos, unique_key) && !compare(unique_key, *pos));
                return false;
            }
            result.insert(pos, std::forward<key_type>(unique_key));
            return true;
        }
    }
    result.push_back(std::forward<key_type>(unique_key)); 
    return true;
}

template<class T, class key_type>
void insertion_sort(T & result, const key_type & value) {
    result.push_back(value);
    auto const left = result.begin();
    auto right = result.end(); --right;
    while (right > left) {
        auto const old = right--;
        if (*old < *right) {
            std::swap(*old, *right);
        }
        else {
            break;
        }
    }
    SDL_ASSERT(left <= right);
    SDL_ASSERT(std::is_sorted(result.cbegin(), result.cend()));
}

template<class T, class key_type, class fun_type>
void insertion_sort(T & result, const key_type & value, fun_type compare) {
    result.push_back(value);
    auto const left = result.begin();
    auto right = result.end(); --right;
    while (right > left) {
        auto const old = right--;
        if (compare(*old, *right)) {
            std::swap(*old, *right);
        }
        else {
            break;
        }
    }
    SDL_ASSERT(left <= right);
    SDL_ASSERT(std::is_sorted(result.cbegin(), result.cend()));
}

} // algo
} // sdl

#endif // __SDL_COMMON_ALGORITHM_H__
