/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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
#include "OgreStableHeaders.h"

#include "OgreStringConverter.h"

#include "OgreCharconv.h"
#include "OgreException.h"
#include "OgrePlatform.h"
#include "OgreString.h"

#include <sstream>

namespace Ogre
{
    String StringConverter::msDefaultStringLocale = OGRE_DEFAULT_LOCALE;
    std::locale StringConverter::msLocale = std::locale( msDefaultStringLocale.c_str() );
    bool StringConverter::msUseLocale = false;

    template <typename T>
    String StringConverter::_toString( T val, unsigned short width, char fill, std::ios::fmtflags flags )
    {
#ifdef OGRE_HAS_CHARCONV
        if( width == 0 && flags == std::ios::fmtflags( 0 ) )
        {
            char buf[32];
            to_chars_result res = to_chars( buf, buf + sizeof( buf ), val );
            assert( res.ec == errc() );
            return String( buf, res.ptr );
        }
#endif
        StringStream stream;
        if( msUseLocale )
            stream.imbue( msLocale );
        stream.width( width );
        stream.fill( fill );
        if( flags & std::ios::basefield )
        {
            stream.setf( flags, std::ios::basefield );
            stream.setf( ( flags & ~std::ios::basefield ) | std::ios::showbase );
        }
        else if( flags )
            stream.setf( flags );
        stream << val;
        return stream.str();
    }
    //-----------------------------------------------------------------------
    template <typename T>
    T StringConverter::_fromString( const String &val, T defaultValue )
    {
#ifdef OGRE_HAS_CHARCONV
        T v;
        from_chars_result res = from_chars_all( val.c_str(), val.c_str() + val.size(), v );
        return res.ec == errc() ? v : defaultValue;
#else
        // Use iStringStream for direct correspondence with toString
        StringStream str( val );
        if( msUseLocale )
            str.imbue( msLocale );
        T ret = defaultValue;
        if( !( str >> ret ) )
            return defaultValue;

        return ret;
#endif
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( float val, unsigned short precision, unsigned short width,
                                      char fill, std::ios::fmtflags flags )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        if( width == 0 && flags == std::ios::fmtflags( 0 ) )
        {
            char buf[32];
            to_chars_result res =
                precision ? to_chars( buf, buf + sizeof( buf ), val, chars_format::general, precision )
                          : to_chars( buf, buf + sizeof( buf ), val );
            assert( res.ec == errc() );
            return String( buf, res.ptr );
        }
#endif
        StringStream stream;
        if( msUseLocale )
            stream.imbue( msLocale );
        if( precision != 0 )
            stream.precision( precision );
        stream.width( width );
        stream.fill( fill );
        if( flags )
            stream.setf( flags );
        stream << val;
        return stream.str();
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( double val, unsigned short precision, unsigned short width,
                                      char fill, std::ios::fmtflags flags )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        if( width == 0 && flags == std::ios::fmtflags( 0 ) )
        {
            char buf[32];
            to_chars_result res =
                precision ? to_chars( buf, buf + sizeof( buf ), val, chars_format::general, precision )
                          : to_chars( buf, buf + sizeof( buf ), val );
            assert( res.ec == errc() );
            return String( buf, res.ptr );
        }
#endif
        StringStream stream;
        if( msUseLocale )
            stream.imbue( msLocale );
        if( precision != 0 )
            stream.precision( precision );
        stream.width( width );
        stream.fill( fill );
        if( flags )
            stream.setf( flags );
        stream << val;
        return stream.str();
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( int val, unsigned short width, char fill,
                                      std::ios::fmtflags flags )
    {
        return _toString( val, width, fill, flags );
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( unsigned int val, unsigned short width, char fill,
                                      std::ios::fmtflags flags )
    {
        return _toString( val, width, fill, flags );
    }
    String StringConverter::toString( unsigned long val, unsigned short width, char fill,
                                      std::ios::fmtflags flags )
    {
        return _toString( val, width, fill, flags );
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( long val, unsigned short width, char fill,
                                      std::ios::fmtflags flags )
    {
        return _toString( val, width, fill, flags );
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( long long val, unsigned short width, char fill,
                                      std::ios::fmtflags flags )
    {
        return _toString( val, width, fill, flags );
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( unsigned long long val, unsigned short width, char fill,
                                      std::ios::fmtflags flags )
    {
        return _toString( val, width, fill, flags );
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( const Vector2 &val )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        char buf[128];
        to_chars_result res = to_chars_all( buf, buf + sizeof( buf ), val.x, val.y );
        assert( res.ec == errc() );
        return String( buf, res.ptr );
#else
        StringStream stream;
        if( msUseLocale )
            stream.imbue( msLocale );
        stream << val.x << " " << val.y;
        return stream.str();
#endif
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( const Vector3 &val )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        char buf[128];
        to_chars_result res = to_chars_all( buf, buf + sizeof( buf ), val.x, val.y, val.z );
        assert( res.ec == errc() );
        return String( buf, res.ptr );
#else
        StringStream stream;
        if( msUseLocale )
            stream.imbue( msLocale );
        stream << val.x << " " << val.y << " " << val.z;
        return stream.str();
#endif
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( const Vector4 &val )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        char buf[128];
        to_chars_result res = to_chars_all( buf, buf + sizeof( buf ), val.x, val.y, val.z, val.w );
        assert( res.ec == errc() );
        return String( buf, res.ptr );
#else
        StringStream stream;
        if( msUseLocale )
            stream.imbue( msLocale );
        stream << val.x << " " << val.y << " " << val.z << " " << val.w;
        return stream.str();
#endif
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( const Matrix3 &val )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        char buf[256];  // 9 * (17 + 1) == 162
        to_chars_result res =
            to_chars_all( buf, buf + sizeof( buf ), val[0][0], val[0][1], val[0][2], val[1][0],
                          val[1][1], val[1][2], val[2][0], val[2][1], val[2][2] );
        assert( res.ec == errc() );
        return String( buf, res.ptr );
#else
        StringStream stream;
        stream.imbue( msLocale );
        stream << val[0][0] << " " << val[0][1] << " " << val[0][2] << " " << val[1][0] << " "
               << val[1][1] << " " << val[1][2] << " " << val[2][0] << " " << val[2][1] << " "
               << val[2][2];
        return stream.str();
#endif
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( bool val, bool yesNo )
    {
        if( val )
        {
            if( yesNo )
            {
                return "yes";
            }
            else
            {
                return "true";
            }
        }
        else if( yesNo )
        {
            return "no";
        }
        else
        {
            return "false";
        }
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( const Matrix4 &val )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        char buf[384];  // 16 * (17 + 1) == 288
        to_chars_result res =
            to_chars_all( buf, buf + sizeof( buf ), val[0][0], val[0][1], val[0][2], val[0][3],
                          val[1][0], val[1][1], val[1][2], val[1][3], val[2][0], val[2][1], val[2][2],
                          val[2][3], val[3][0], val[3][1], val[3][2], val[3][3] );
        assert( res.ec == errc() );
        return String( buf, res.ptr );
#else
        StringStream stream;
        stream.imbue( msLocale );
        stream << val[0][0] << " " << val[0][1] << " " << val[0][2] << " " << val[0][3] << " "
               << val[1][0] << " " << val[1][1] << " " << val[1][2] << " " << val[1][3] << " "
               << val[2][0] << " " << val[2][1] << " " << val[2][2] << " " << val[2][3] << " "
               << val[3][0] << " " << val[3][1] << " " << val[3][2] << " " << val[3][3];
        return stream.str();
#endif
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( const Quaternion &val )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        char buf[128];
        to_chars_result res = to_chars_all( buf, buf + sizeof( buf ), val.w, val.x, val.y, val.z );
        assert( res.ec == errc() );
        return String( buf, res.ptr );
#else
        StringStream stream;
        if( msUseLocale )
            stream.imbue( msLocale );
        stream << val.w << " " << val.x << " " << val.y << " " << val.z;
        return stream.str();
#endif
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( const ColourValue &val )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        char buf[128];
        to_chars_result res = to_chars_all( buf, buf + sizeof( buf ), val.r, val.g, val.b, val.a );
        assert( res.ec == errc() );
        return String( buf, res.ptr );
#else
        StringStream stream;
        if( msUseLocale )
            stream.imbue( msLocale );
        stream << val.r << " " << val.g << " " << val.b << " " << val.a;
        return stream.str();
#endif
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( const StringVector &val )
    {
        StringStream stream;
        if( msUseLocale )
            stream.imbue( msLocale );
        StringVector::const_iterator i, iend, ibegin;
        ibegin = val.begin();
        iend = val.end();
        for( i = ibegin; i != iend; ++i )
        {
            if( i != ibegin )
                stream << " ";

            stream << *i;
        }
        return stream.str();
    }
    //-----------------------------------------------------------------------
    Real StringConverter::parseReal( const String &val, Real defaultValue )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        Real v;
        from_chars_result res = from_chars_all( val.c_str(), val.c_str() + val.size(), v );
        return res.ec == errc() ? v : defaultValue;
#else
        // Use iStringStream for direct correspondence with toString
        StringStream str( val );
        if( msUseLocale )
            str.imbue( msLocale );
        Real ret = defaultValue;
        if( !( str >> ret ) )
            return defaultValue;

        return ret;
#endif
    }
    //-----------------------------------------------------------------------
    short StringConverter::parseShort( const String &val, short defaultValue )
    {
        return _fromString( val, defaultValue );
    }
    //-----------------------------------------------------------------------
    unsigned short StringConverter::parseUnsignedShort( const String &val, unsigned short defaultValue )
    {
        return _fromString( val, defaultValue );
    }
    //-----------------------------------------------------------------------
    int StringConverter::parseInt( const String &val, int defaultValue )
    {
        return _fromString( val, defaultValue );
    }
    //-----------------------------------------------------------------------
    unsigned int StringConverter::parseUnsignedInt( const String &val, unsigned int defaultValue )
    {
        return _fromString( val, defaultValue );
    }
    //-----------------------------------------------------------------------
    long StringConverter::parseLong( const String &val, long defaultValue )
    {
        return _fromString( val, defaultValue );
    }
    //-----------------------------------------------------------------------
    unsigned long StringConverter::parseUnsignedLong( const String &val, unsigned long defaultValue )
    {
        return _fromString( val, defaultValue );
    }
    //-----------------------------------------------------------------------
    size_t StringConverter::parseSizeT( const String &val, size_t defaultValue )
    {
        return _fromString( val, defaultValue );
    }
    //-----------------------------------------------------------------------
    bool StringConverter::parseBool( const String &val, bool defaultValue, bool *error )
    {
        if( ( StringUtil::startsWith( val, "true" ) || StringUtil::startsWith( val, "yes" ) ||
              StringUtil::startsWith( val, "1" ) || StringUtil::startsWith( val, "on" ) ) )
        {
            if( error )
                *error = false;

            return true;
        }
        else if( ( StringUtil::startsWith( val, "false" ) || StringUtil::startsWith( val, "no" ) ||
                   StringUtil::startsWith( val, "0" ) || StringUtil::startsWith( val, "off" ) ) )
        {
            if( error )
                *error = false;

            return false;
        }
        else
        {
            if( error )
                *error = true;

            return defaultValue;
        }
    }
    //-----------------------------------------------------------------------
    Vector2 StringConverter::parseVector2( const String &val, const Vector2 &defaultValue )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        Vector2 v;
        from_chars_result res = from_chars_all( val.c_str(), val.c_str() + val.size(), v.x, v.y );
        return res.ec == errc() ? v : defaultValue;
#else
        // Split on space
        vector<String>::type vec = StringUtil::split( val );

        if( vec.size() != 2 )
        {
            return defaultValue;
        }
        else
        {
            return Vector2( parseReal( vec[0], defaultValue[0] ), parseReal( vec[1], defaultValue[1] ) );
        }
#endif
    }
    //-----------------------------------------------------------------------
    Vector3 StringConverter::parseVector3( const String &val, const Vector3 &defaultValue )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        Vector3 v;
        from_chars_result res = from_chars_all( val.c_str(), val.c_str() + val.size(), v.x, v.y, v.z );
        return res.ec == errc() ? v : defaultValue;
#else
        // Split on space
        vector<String>::type vec = StringUtil::split( val );

        if( vec.size() != 3 )
        {
            return defaultValue;
        }
        else
        {
            return Vector3( parseReal( vec[0], defaultValue[0] ), parseReal( vec[1], defaultValue[1] ),
                            parseReal( vec[2], defaultValue[2] ) );
        }
#endif
    }
    //-----------------------------------------------------------------------
    Vector4 StringConverter::parseVector4( const String &val, const Vector4 &defaultValue )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        Vector4 v;
        from_chars_result res =
            from_chars_all( val.c_str(), val.c_str() + val.size(), v.x, v.y, v.z, v.w );
        return res.ec == errc() ? v : defaultValue;
#else
        // Split on space
        vector<String>::type vec = StringUtil::split( val );

        if( vec.size() != 4 )
        {
            return defaultValue;
        }
        else
        {
            return Vector4( parseReal( vec[0], defaultValue[0] ), parseReal( vec[1], defaultValue[1] ),
                            parseReal( vec[2], defaultValue[2] ), parseReal( vec[3], defaultValue[3] ) );
        }
#endif
    }
    //-----------------------------------------------------------------------
    Matrix3 StringConverter::parseMatrix3( const String &val, const Matrix3 &defaultValue )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        Matrix3 v;
        from_chars_result res =
            from_chars_all( val.c_str(), val.c_str() + val.size(), v[0][0], v[0][1], v[0][2], v[1][0],
                            v[1][1], v[1][2], v[2][0], v[2][1], v[2][2] );
        return res.ec == errc() ? v : defaultValue;
#else
        // Split on space
        vector<String>::type vec = StringUtil::split( val );

        if( vec.size() != 9 )
        {
            return defaultValue;
        }
        else
        {
            return Matrix3(
                parseReal( vec[0], defaultValue[0][0] ), parseReal( vec[1], defaultValue[0][1] ),
                parseReal( vec[2], defaultValue[0][2] ),

                parseReal( vec[3], defaultValue[1][0] ), parseReal( vec[4], defaultValue[1][1] ),
                parseReal( vec[5], defaultValue[1][2] ),

                parseReal( vec[6], defaultValue[2][0] ), parseReal( vec[7], defaultValue[2][1] ),
                parseReal( vec[8], defaultValue[2][2] ) );
        }
#endif
    }
    //-----------------------------------------------------------------------
    Matrix4 StringConverter::parseMatrix4( const String &val, const Matrix4 &defaultValue )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        Matrix4 v;
        from_chars_result res = from_chars_all(
            val.c_str(), val.c_str() + val.size(), v[0][0], v[0][1], v[0][2], v[0][3], v[1][0], v[1][1],
            v[1][2], v[1][3], v[2][0], v[2][1], v[2][2], v[2][3], v[3][0], v[3][1], v[3][2], v[3][3] );
        return res.ec == errc() ? v : defaultValue;
#else
        // Split on space
        vector<String>::type vec = StringUtil::split( val );

        if( vec.size() != 16 )
        {
            return defaultValue;
        }
        else
        {
            return Matrix4(
                parseReal( vec[0], defaultValue[0][0] ), parseReal( vec[1], defaultValue[0][1] ),
                parseReal( vec[2], defaultValue[0][2] ), parseReal( vec[3], defaultValue[0][3] ),

                parseReal( vec[4], defaultValue[1][0] ), parseReal( vec[5], defaultValue[1][1] ),
                parseReal( vec[6], defaultValue[1][2] ), parseReal( vec[7], defaultValue[1][3] ),

                parseReal( vec[8], defaultValue[2][0] ), parseReal( vec[9], defaultValue[2][1] ),
                parseReal( vec[10], defaultValue[2][2] ), parseReal( vec[11], defaultValue[2][3] ),

                parseReal( vec[12], defaultValue[3][0] ), parseReal( vec[13], defaultValue[3][1] ),
                parseReal( vec[14], defaultValue[3][2] ), parseReal( vec[15], defaultValue[3][3] ) );
        }
#endif
    }
    //-----------------------------------------------------------------------
    Quaternion StringConverter::parseQuaternion( const String &val, const Quaternion &defaultValue )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        Quaternion v;
        from_chars_result res =
            from_chars_all( val.c_str(), val.c_str() + val.size(), v.w, v.x, v.y, v.z );
        return res.ec == errc() ? v : defaultValue;
#else
        // Split on space
        vector<String>::type vec = StringUtil::split( val );

        if( vec.size() != 4 )
        {
            return defaultValue;
        }
        else
        {
            return Quaternion(
                parseReal( vec[0], defaultValue[0] ), parseReal( vec[1], defaultValue[1] ),
                parseReal( vec[2], defaultValue[2] ), parseReal( vec[3], defaultValue[3] ) );
        }
#endif
    }
    //-----------------------------------------------------------------------
    ColourValue StringConverter::parseColourValue( const String &val, const ColourValue &defaultValue )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        ColourValue v;
        from_chars_result res = from_chars_all( val.c_str(), val.c_str() + val.size(), v.r, v.g, v.b );
        if( res.ec == errc() && res.ptr != val.c_str() + val.size() )
            res = from_chars_all( res.ptr, val.c_str() + val.size(), v.a );
        return res.ec == errc() ? v : defaultValue;
#else
        // Split on space
        vector<String>::type vec = StringUtil::split( val );

        if( vec.size() == 4 )
        {
            return ColourValue(
                parseReal( vec[0], defaultValue[0] ), parseReal( vec[1], defaultValue[1] ),
                parseReal( vec[2], defaultValue[2] ), parseReal( vec[3], defaultValue[3] ) );
        }
        else if( vec.size() == 3 )
        {
            return ColourValue( parseReal( vec[0], defaultValue[0] ),
                                parseReal( vec[1], defaultValue[1] ),
                                parseReal( vec[2], defaultValue[2] ), 1.0f );
        }
        else
        {
            return defaultValue;
        }
#endif
    }
    //-----------------------------------------------------------------------
    StringVector StringConverter::parseStringVector( const String &val )
    {
        return StringUtil::split( val );
    }
    //-----------------------------------------------------------------------
    bool StringConverter::isNumber( const String &val )
    {
#ifdef OGRE_HAS_CHARCONV_FLOAT
        double d;
        int64_t i64;
        uint64_t u64;
        const char *first = val.c_str(), *last = first + val.size();
        from_chars_result res = from_chars( first, last, d );
        if( res.ec != errc() )
            res = from_chars( first, last, i64 );
        if( res.ec != errc() )
            res = from_chars( first, last, u64 );
        return res.ec == errc() && res.ptr == last;
#else
        StringStream str( val );
        if( msUseLocale )
            str.imbue( msLocale );
        float tst;
        str >> tst;
        return !str.fail() && str.eof();
#endif
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( ColourBufferType val )
    {
        String str;
        switch( val )
        {
        case CBT_BACK:
            str = "Back";
            break;
        case CBT_BACK_LEFT:
            str = "Back Left";
            break;
        case CBT_BACK_RIGHT:
            str = "Back Right";
            break;
        default:
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "Unsupported colour buffer value",
                         "StringConverter::toString(const ColourBufferType& val)" );
        }

        return str;
    }
    //-----------------------------------------------------------------------
    ColourBufferType StringConverter::parseColourBuffer( const String &val,
                                                         ColourBufferType defaultValue )
    {
        ColourBufferType result = defaultValue;
        if( val.compare( "Back" ) == 0 )
        {
            result = CBT_BACK;
        }
        else if( val.compare( "Back Left" ) == 0 )
        {
            result = CBT_BACK_LEFT;
        }
        else if( val.compare( "Back Right" ) == 0 )
        {
            result = CBT_BACK_RIGHT;
        }

        return result;
    }
    //-----------------------------------------------------------------------
    String StringConverter::toString( StereoModeType val )
    {
        String str;
        switch( val )
        {
        case SMT_NONE:
            str = "None";
            break;
        case SMT_FRAME_SEQUENTIAL:
            str = "Frame Sequential";
            break;
        default:
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "Unsupported stereo mode value",
                         "StringConverter::toString(const StereoModeType& val)" );
        }

        return str;
    }
    //-----------------------------------------------------------------------
    StereoModeType StringConverter::parseStereoMode( const String &val, StereoModeType defaultValue )
    {
        StereoModeType result = defaultValue;
        if( val.compare( "None" ) == 0 )
        {
            result = SMT_NONE;
        }
        else if( val.compare( "Frame Sequential" ) == 0 )
        {
            result = SMT_FRAME_SEQUENTIAL;
        }

        return result;
    }
    //-----------------------------------------------------------------------
}  // namespace Ogre
