// algorithm.h
//
#pragma once
#ifndef __SDL_COMMON_ALGORITHM_H__
#define __SDL_COMMON_ALGORITHM_H__

#include <algorithm>

namespace sdl { namespace algo { namespace scope_exit { 

template<typename fun_type>
struct scope_guard {
    explicit scope_guard(fun_type && f) : fun(std::move(f)) {}
    ~scope_guard() {
        fun();
    }
private:
    fun_type fun;
};

template <typename T> inline
scope_guard<T> create_scope_guard(T&& exitScope) {
    return scope_guard<T>(std::forward<T>(exitScope));
}

#if SDL_DEBUG
template<typename fun_type>
struct assert_guard {
    explicit assert_guard(fun_type && f) : fun(std::move(f)) {}
    ~assert_guard() { 
        SDL_ASSERT(fun());
    }
private:
    fun_type fun;
};
#else
template<typename fun_type>
struct assert_guard {
    explicit assert_guard(fun_type &&){}
};
#endif

template <typename T> inline
assert_guard<T> create_assert_guard(T&& exitScope) {
    return assert_guard<T>(std::forward<T>(exitScope));
}

} // scope_exit

#define _SDL_UTILITY_EXIT_SCOPE_LINENAME_CAT(name, line) name##line
#define _SDL_UTILITY_EXIT_SCOPE_LINENAME(name, line) _SDL_UTILITY_EXIT_SCOPE_LINENAME_CAT(name, line)

#define SDL_UTILITY_SCOPE_EXIT(f) const auto& _SDL_UTILITY_EXIT_SCOPE_LINENAME(scope_exit_line_, __LINE__) = scope_exit::create_scope_guard(f)
#define SDL_UTILITY_ASSERT_EXIT(f) const auto& _SDL_UTILITY_EXIT_SCOPE_LINENAME(scope_exit_line_, __LINE__) = scope_exit::create_assert_guard(f)

#if SDL_DEBUG
#define ASSERT_SCOPE_EXIT(...)  SDL_UTILITY_ASSERT_EXIT(__VA_ARGS__)
#else
#define ASSERT_SCOPE_EXIT(...)  ((void)0)
#endif

#if SDL_DEBUG > 1
#define ASSERT_SCOPE_EXIT_DEBUG_2(...)  ASSERT_SCOPE_EXIT(__VA_ARGS__)
#else
#define ASSERT_SCOPE_EXIT_DEBUG_2(...)  ((void)0)
#endif

template<class T, class key_type>
inline bool is_find(T const & result, key_type const & value) {
    return std::find(std::begin(result), std::end(result), value) != std::end(result);
}

template<class T>
inline bool is_sorted(T const & result) {
    return std::is_sorted(std::begin(result), std::end(result));
}

template<class T, class fun_type>
inline bool is_sorted(T const & result, fun_type compare) {
    return std::is_sorted(std::begin(result), std::end(result), compare);
}

template<class T, class key_type>
bool binary_insertion(T & result, key_type && unique_key) {
    ASSERT_SCOPE_EXIT_DEBUG_2([&result]{
        return is_sorted(result);
    });
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
    ASSERT_SCOPE_EXIT_DEBUG_2([&result, compare]{
        return is_sorted(result, compare);
    });
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
    ASSERT_SCOPE_EXIT_DEBUG_2([&result]{
        return is_sorted(result);
    });
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
}

template<class T, class key_type, class fun_type>
void insertion_sort(T & result, const key_type & value, fun_type compare) {
    ASSERT_SCOPE_EXIT_DEBUG_2([&result, compare]{
        return is_sorted(result, compare);
    });
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
}

} // algo
} // sdl

#endif // __SDL_COMMON_ALGORITHM_H__
