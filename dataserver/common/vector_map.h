// vector_map.h
//
#pragma once
#ifndef __SDL_COMMON_VECTOR_MAP_H__
#define __SDL_COMMON_VECTOR_MAP_H__

namespace sdl { namespace db {

template<class Key, class T>
class vector_map {
public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = std::pair<key_type, mapped_type>;
private:
    using vector_type = std::vector<value_type>;
    vector_type data;
public:
    using const_iterator = typename vector_type::const_iterator;
    vector_map() = default;

    const_iterator begin() const {
        return data.begin();
    }
    const_iterator end() const {
        return data.end();
    }
    const_iterator find(const key_type & k) const {
        auto const i = std::lower_bound(data.begin(), data.end(), k, 
            [](value_type const & x, const key_type & k) {
            return x.first < k;
        });
        if ((i != data.end()) && !(k < i->first)) {
            return i;
        }
        return data.end();
    }
    mapped_type & operator[](const key_type & k) {
        auto const i = std::lower_bound(data.begin(), data.end(), k, 
            [](value_type const & x, const key_type & k) {
            return x.first < k;
        });
        if (i != data.end()) {
            if (!(k < i->first)) {
                return i->second;
            }
            return data.emplace(i, k, T{})->second;
        }
        else {
            data.emplace_back(k, T{});
            return data.back().second;
        }
    }
};

template<class Key, class Compare = std::less<Key>>
class vector_set { //FIXME: to be tested
    using vector_type = std::vector<Key>;
    vector_type data;
public:
    using key_type = Key;
    using iterator = typename vector_type::iterator;
    using const_iterator = typename vector_type::const_iterator;
private:
    static bool is_same(const key_type & lhs, const key_type & rhs) {
        return !(Compare(lhs, rhs) || Compare(rhs, lhs));
    }
public:
    vector_set() = default;
    vector_set(vector_set && src): data(std::move(src.data)) {}
    void swap(vector_set & src) {
        data.swap(src.data);
    }
    iterator lower_bound(key_type const & value) {
        return std::lower_bound(data.begin(), data.end(), value, Compare());
    }
    bool insert(key_type const & value) {
        iterator const pos = lower_bound(value);
        if (pos != data.end()) {
            if (is_same(*pos, value)) {
                return false;
            }
            data.insert(pos, value);
            return true;
        }
        data.push_back(value);
        return true;
    }
    iterator erase(iterator const & it) {
        SDL_ASSERT(it != data.end());
        size_t const pos = std::distance(data.begin(), it);
        data.erase(it);
        return data.begin() + pos;
    }
    bool empty() const {
        return data.empty();
    }
    void clear() {
        data.clear();
    }
    size_t size() const {
        return data.size();
    }
    iterator begin() {
        return data.begin();
    }
    iterator end() {
        return data.end();
    }
    const_iterator cbegin() const {
        return data.cbegin();
    }
    const_iterator cend() const {
        return data.cend();
    }
private:
    bool insert(iterator, key_type const & value) {
        return false;
    }
};

} // db
} // sdl

#endif // __SDL_COMMON_VECTOR_MAP_H__