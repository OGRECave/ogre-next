#ifndef _OgreStlCharconv_H_
#define _OgreStlCharconv_H_

#if defined (_MSVC_LANG) && _MSVC_LANG >= 201703L // MSVC in C++17 mode
#include <charconv>
#define OGRE_HAS_CHARCONV
#define OGRE_HAS_CHARCONV_FLOAT
#endif


#ifdef OGRE_HAS_CHARCONV

namespace Ogre
{
	using std::from_chars_result;
	using std::from_chars;
	using std::to_chars_result;
	using std::to_chars;
	using std::chars_format;
	using std::errc;
}

// some utilities
namespace Ogre
{
	namespace
	{
		constexpr bool is_space(char c) noexcept { return c == ' ' || ((unsigned)(c - '\t') <= (unsigned)('\r' - '\t')); } // 0x20' ', 0x9'\t', 0xA'\n', 0xB'\v', 0xC'\f', 0xD'\r'
		constexpr void skip_space(const char*& ptr, const char* last, char SEP) { while (ptr < last && *ptr != SEP && is_space(*ptr)) ++ptr; }
		constexpr bool skip_sep(const char*& ptr, const char* last, char SEP) { skip_space(ptr, last, SEP); return ptr < last && *ptr == SEP ? ++ptr, true : false; }
	}

	/** Scans SEP separated arguments list from the buffer, skipping spaces. */
	template<char SEP = ' ', typename... Args>
	from_chars_result from_chars_all(const char* first, const char* last, Args&... args) {
		from_chars_result res = { first, errc() };
		return (... && ((res.ptr == first || skip_sep(res.ptr, last, SEP)) && (skip_space(res.ptr, last, SEP), res = from_chars(res.ptr, last, args), res.ec == errc())))
			? skip_space(res.ptr, last, SEP), res : from_chars_result{ first, errc::invalid_argument };
	}

	/** Prints SEP separated arguments list into the buffer, with smallest possible precision necessary to restore values exactly. Buffer is NOT zero-terminated. */
	template<char SEP = ' ', typename... Args>
	to_chars_result to_chars_all(char* first, char* last, Args... args) {
		to_chars_result res = { first, errc() };
		return (... && ((res = to_chars(res.ptr, last, args), res.ec == errc()) && res.ptr < last && (*res.ptr++ = SEP)))
			? res : to_chars_result{ last, errc::value_too_large };
	}
}

#endif

#endif
