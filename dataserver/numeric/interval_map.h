// interval_map.h
//
#pragma once
#ifndef __SDL_NUMERIC_INTERVAL_MAP_H__
#define __SDL_NUMERIC_INTERVAL_MAP_H__

#include "dataserver/common/common.h"
#include <map>
#include <limits>

namespace sdl {

template<class K, class V>
class interval_map : sdl::noncopyable {
    using map_type = std::map<K,V>;
    //using iterator = typename map_type::iterator;
    using value_type = typename map_type::value_type;
    map_type m_map;
public:
    // constructor associates whole range of K with val
    // by inserting (K_min, val) into the map
    interval_map(K const& K_min, V const& val);
    explicit interval_map(V const& val);

#if SDL_DEBUG
    map_type const & map() const {
        return m_map;
    }
#endif

    // Assign value val to interval [keyBegin, keyEnd). 
    // Overwrite previous values in this interval. 
    // Do not change values outside this interval.
    // The interval includes keyBegin, but excludes keyEnd.
    void assign( K const& keyBegin, K const& keyEnd, const V& val );

    // look-up of the value associated with key
    V const& operator[]( K const& key ) const {
        return ( --m_map.upper_bound(key) )->second;
    }
};

template<class K, class V>
interval_map<K, V>::interval_map(K const& K_min, V const& val) {
    m_map.emplace(K_min, val);
}

template<class K, class V>
interval_map<K, V>::interval_map( V const& val)
    : interval_map(std::numeric_limits<K>::min(), val)
{
}

template<class K, class V>
void interval_map<K, V>::assign( K const& keyBegin, K const& keyEnd, const V& val ) {

	if (!(keyBegin < keyEnd)) return; //empty interval

    SDL_ASSERT(!m_map.empty());
    SDL_ASSERT(!(keyBegin < m_map.begin()->first));
    SDL_ASSERT(!(keyEnd < m_map.begin()->first));

	auto b1 = m_map.upper_bound(keyBegin);
	auto b2 = m_map.upper_bound(keyEnd);

	auto l = b1; --l;
	auto r = b2; --r;

	if (!(l->first < keyBegin))
	{
		if (l != m_map.begin())
		{
			auto prev = l; --prev;
			if (prev->second == val)
			{
				--l;
				--b1;
			}
		}
	}

	if (l == r)
	{
		if (l->second == val)
			return;

		if (l->first < keyBegin)
		{
			m_map.emplace_hint(m_map.emplace_hint(l, keyBegin, val), keyEnd, l->second);
		}
		else
		{
			m_map.emplace_hint(l, keyEnd, l->second);
			l->second = val;
		}
	}
	else
	{
		if (r->second == val)
		{
			m_map.erase(b1, b2);
		}
		else
		{
			if (r->first < keyEnd)
			{
				m_map.erase(b1, m_map.emplace_hint(r, keyEnd, r->second));
			}
			else
			{
				m_map.erase(b1, r);
			}
		}
		if (l->second == val)
			return;

		if (l->first < keyBegin)
			m_map.emplace_hint(l, keyBegin, val);
		else
			l->second = val;
	}
}

} // sdl

#endif // __SDL_NUMERIC_INTERVAL_MAP_H__
