/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#ifndef _OgreCharconv_H_
#define _OgreCharconv_H_

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
