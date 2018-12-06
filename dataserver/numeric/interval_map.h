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
class interval_map {
    using map_type = std::map<K,V>;
    map_type m_map;
public:
    using key_type = K;
    using value_type = V;

    // constructor associates whole range of K with val by inserting (K_min, val)
    // into the map
    explicit interval_map( V const& val);

#if SDL_DEBUG
    map_type const & map() const {
        return m_map;
    }
#endif

    // Assign value val to interval [keyBegin, keyEnd). 
    // Overwrite previous values in this interval. 
    // Do not change values outside this interval.
    // Conforming to the C++ Standard Library conventions, the interval 
    // includes keyBegin, but excludes keyEnd.
    // If !( keyBegin < keyEnd ), this designates an empty interval, 
    // and assign must do nothing.
    void assign( K const& keyBegin, K const& keyEnd, const V& val );

    // look-up of the value associated with key
    V const& operator[]( K const& key ) const {
        return ( --m_map.upper_bound(key) )->second;
    }
};

template<class K, class V>
interval_map<K, V>::interval_map( V const& val) {
    m_map.insert(m_map.begin(), std::make_pair(std::numeric_limits<K>::min(),val));
};

template<class K, class V>
void interval_map<K, V>::assign( K const& keyBegin, K const& keyEnd, const V& val ) {

	if (!(keyBegin < keyEnd)) return; //empty interval

	typedef std::map<K, V> map_type;
	typedef map_type::iterator iterator;
	typedef map_type::value_type value_type;

	iterator b1 = m_map.upper_bound(keyBegin);
	iterator b2 = m_map.upper_bound(keyEnd);

	iterator l = b1; --l;
	iterator r = b2; --r;

	if (!(l->first < keyBegin))
	{
		if (l != m_map.begin())
		{
			iterator prev = l; --prev;
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
			m_map.insert(m_map.insert(l, value_type(keyBegin, val)), 
				value_type(keyEnd, l->second));
		}
		else
		{
			m_map.insert(l, value_type(keyEnd, l->second));
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
				m_map.erase(b1, m_map.insert(r, value_type(keyEnd, r->second)));
			}
			else
			{
				m_map.erase(b1, r);
			}
		}
		if (l->second == val)
			return;

		if (l->first < keyBegin)
			m_map.insert(l, value_type(keyBegin, val));
		else
			l->second = val;
	}
}

} // sdl

#endif // __SDL_NUMERIC_INTERVAL_MAP_H__
