// cyrillic.h
#pragma once
#ifndef __SDL_UTILS_CYRILLIC_H__
#define __SDL_UTILS_CYRILLIC_H__

#include "dataserver/utils/conv.h"
#include <algorithm>

namespace sdl { namespace utils {

struct cyrillic {
    //http://en.wikipedia.org/wiki/Cyrillic_script_in_Unicode
    enum enum_type {
          Cyr_A  = 1040 //L'А' = 0x0410
	    , Cyr_JA = 1071 //L'Я' = 0x042F
	    , Cyr_a  = 1072 //L'а' = 0x0430
	    , Cyr_ja = 1103 //L'я' = 0x044F
	    , Cyr_JO = 1025 //L'Ё' = 0x0401
	    , Cyr_jo = 1105 //L'ё' = 0x0451
        , Cyr_E  = 1045 //L'Е' = 0x0415
        , Cyr_e  = 1077 //L'е' = 0x0435
    	, Cyr_delta = Cyr_a - Cyr_A
    };
#if defined(SDL_OS_WIN32)
    static_assert(Cyr_A  == L'А', "");
	static_assert(Cyr_JA == L'Я', "");
	static_assert(Cyr_a  == L'а', "");
	static_assert(Cyr_ja == L'я', "");
	static_assert(Cyr_JO == L'Ё', "");
	static_assert(Cyr_jo == L'ё', "");
    static_assert(Cyr_E == L'Е', "");
    static_assert(Cyr_e == L'е', "");
#endif
    static wchar_t to_upper(const wchar_t character) {
	    static_assert(Cyr_delta == 32, "");
        if ((Cyr_a <= character) && (character <= Cyr_ja))
			return character - Cyr_delta;
		if (Cyr_jo == character)
			return Cyr_JO;
        return ::towupper(character);
    }
    static wchar_t to_upper_without_jo(const wchar_t character) {
        if ((Cyr_jo == character) || ( Cyr_JO == character))
            return Cyr_E;
        if ((Cyr_a <= character) && (character <= Cyr_ja))
			return character - Cyr_delta;
        return ::towupper(character);
    }
    static wchar_t to_lower(const wchar_t character) {
	    static_assert(Cyr_delta == 32, "");
        if ((Cyr_A <= character) && (character <= Cyr_JA))
			return character + Cyr_delta;
		if (Cyr_JO == character)
			return Cyr_jo;
        return ::towlower(character);
    }
    static wchar_t to_lower_without_jo(const wchar_t character) {
        if ((Cyr_jo == character) || ( Cyr_JO == character))
            return Cyr_e;
        if ((Cyr_A <= character) && (character <= Cyr_JA))
			return character + Cyr_delta;
        return ::towlower(character);
    }
};

struct a_towupper { // convert to upper-case, consider cyrillic unicode
    wchar_t operator()(const wchar_t character) const {
        return cyrillic::to_upper(character);
    }
};

struct a_towupper_without_jo { // convert to upper-case, consider cyrillic unicode
    wchar_t operator()(const wchar_t character) const {
        return cyrillic::to_upper_without_jo(character);
    }
};

//----------------------------------------------------

struct a_towlower { // convert to lower-case, consider cyrillic unicode
    wchar_t operator()(const wchar_t character) const {
        return cyrillic::to_lower(character);
    }
};

struct a_towlower_without_jo { // convert to lower-case, consider cyrillic unicode
    wchar_t operator()(const wchar_t character) const {
        return cyrillic::to_lower_without_jo(character);
    }
};

//------------------------------------------------------------------------

template<class convert_type>
struct a_upper_t : is_static {
    using this_type = a_upper_t;
    static std::wstring convert(std::wstring const & s) {
        std::wstring t(s);
        std::transform(t.begin(), t.end(), t.begin(), convert_type());
        return t;
    }
    static std::wstring & transform(std::wstring & s) {
        std::transform(s.begin(), s.end(), s.begin(), convert_type());
        return s;
    }
    // case insensitive, s2 must be in upper-case
    static bool match_ci(std::wstring && s1, std::wstring const & s2) {
        SDL_ASSERT(s2 == this_type::convert(s2));
        if (s1.size() >= s2.size()) {
            if (this_type::transform(s1).find(s2) != std::wstring::npos) {
                return true;
            }
        }
        return false;
    }
    static bool utf8_match_ci(std::string const & s1, std::wstring const & s2) {
        return this_type::match_ci(db::conv::utf8_to_wide(s1), s2);
    }
    static std::wstring utf8_convert(std::string const & s) {
        return this_type::convert(db::conv::utf8_to_wide(s));
    }
};

using a_upper_without_jo = a_upper_t<a_towupper_without_jo>;

//------------------------------------------------------------------------

template<class convert_type>
struct a_lower_t : is_static {
    using this_type = a_lower_t;
    static std::wstring convert(std::wstring const & s) {
        std::wstring t(s);
        std::transform(t.begin(), t.end(), t.begin(), convert_type());
        return t;
    }
    static std::wstring & transform(std::wstring & s) {
        std::transform(s.begin(), s.end(), s.begin(), convert_type());
        return s;
    }
    // case insensitive, s2 must be in lower-case
    static bool match_ci(std::wstring && s1, std::wstring const & s2) {
        SDL_ASSERT(s2 == this_type::convert(s2));
        if (s1.size() >= s2.size()) {
            if (this_type::transform(s1).find(s2) != std::wstring::npos) {
                return true;
            }
        }
        return false;
    }
    static bool utf8_match_ci(std::string const & s1, std::wstring const & s2) {
        return this_type::match_ci(db::conv::utf8_to_wide(s1), s2);
    }
    static std::wstring utf8_convert(std::string const & s) {
        return this_type::convert(db::conv::utf8_to_wide(s));
    }
};

using a_lower_without_jo = a_lower_t<a_towlower_without_jo>;

//------------------------------------------------------------------------

} // utils
} // sdl

#endif // __SDL_UTILS_CYRILLIC_H__