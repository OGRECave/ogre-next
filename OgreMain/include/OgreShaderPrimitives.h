/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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
#ifndef _OgreShaderPrimitives_H_
#define _OgreShaderPrimitives_H_

#include "OgrePrerequisites.h"

#include "OgreMatrix4.h"
#include "OgreVector2.h"
#include "OgreVector3.h"
#include "OgreVector4.h"

namespace Ogre
{
    template <typename T>
    struct type4
    {
        T x, y, z, w;

        type4() {}
        type4( const Vector4 &val ) :
            x( static_cast<T>( val.x ) ),
            y( static_cast<T>( val.y ) ),
            z( static_cast<T>( val.z ) ),
            w( static_cast<T>( val.w ) )
        {
        }
        type4( const Vector2 &valXY, const Vector2 &valZW ) :
            x( static_cast<T>( valXY.x ) ),
            y( static_cast<T>( valXY.y ) ),
            z( static_cast<T>( valZW.x ) ),
            w( static_cast<T>( valZW.y ) )
        {
        }
    };

    struct float2
    {
        float x, y;
        float2() {}
        float2( const Vector2 &val ) : x( val.x ), y( val.y ) {}
    };
    struct float4 : public type4<float>
    {
        float4() {}
        float4( const Vector4 &val ) : type4( val ) {}
        float4( const Vector2 &valXY, const Vector2 &valZW ) : type4( valXY, valZW ) {}
    };
    struct uint4 : public type4<uint32>
    {
        uint4() {}
        uint4( const Vector4 &val ) : type4( val ) {}
        uint4( const Vector2 &valXY, const Vector2 &valZW ) : type4( valXY, valZW ) {}
    };

    struct float4x4
    {
        union {
            float m[4][4];
            float _m[16];
        };

        float4x4() {}
        float4x4( const Matrix4 &val )
        {
            for( size_t i = 0u; i < 16u; ++i )
                _m[i] = static_cast<float>( val[0][i] );
        }
    };
}  // namespace Ogre

#endif
