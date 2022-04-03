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
#ifndef _OgreShaderParams_H_
#define _OgreShaderParams_H_

#include "OgreGpuProgramParams.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Component
     *  @{
     */
    /** \addtogroup Material
     *  @{
     */

    /** The purpose of this class is to contain a set of both auto and
        manual parameters that may apply to multiple shaders; without
        having the shader to be created first (the drawback of GpuProgramParameters).
    @par
        Useful when loading an Hlms material via JSON (e.g. a Compute Job)
        which may generate multiple shaders depending on the property settings.
    @par
        Parameters are kept unsorted in mParams
    */
    class _OgreExport ShaderParams : public OgreAllocatedObj
    {
    public:
        enum ElementType
        {
            ElementInt,
            ElementUInt,
            ElementFloat
        };

        struct AutoParam
        {
            ElementType                            extraParamType;
            double                                 extraParamValue;
            GpuProgramParameters::AutoConstantType acType;
        };
        struct ManualParam
        {
            ElementType elementType;
            /// Size in bytes from dataBytes
            uint8 dataSizeBytes;
            /// Enough data to cover up to a 4x4 matrix.
            /// Gets reinterpret_cast'ed based on elementType
            uint32 dataBytes[16];
        };
        struct ManualParamEx
        {
            ElementType elementType;
            /// Size in bytes from dataBytes
            uint16 dataSizeBytes;
            /// Pointer to externally owned memory. We won't free it.
            void const *dataBytes;
        };
        struct _OgreExport Param
        {
            String name;
            bool   isAutomatic;
            bool   isEx;
            bool   isDirty;

            union
            {
                AutoParam     ap;
                ManualParam   mp;
                ManualParamEx mpEx;
            };

            template <typename T>
            void setManualValue( T value, uint32 numValues, ElementType elementType );

            void setManualValue( const Vector2 &value );
            void setManualValue( const Vector3 &value );
            void setManualValue( const Vector4 &value );
            void setManualValue( const Matrix3 &value );
            void setManualValue( const Matrix4 &value );

            /** Shortcut for setting the given value without dealing with mp manually.
            @remarks
                You can't write more than 64 bytes of data per parameter.
                (i.e. 16 uint32, 16 int32, or 16 floats)
                Remember to call @see ShaderParams::setDirty otherwise
                changes won't take effect.
            */
            void setManualValue( float value );
            void setManualValue( const float *value, uint32 numValues );
            void setManualValue( int32 value );
            void setManualValue( const int32 *value, uint32 numValues );
            void setManualValue( uint32 value );
            void setManualValue( const uint32 *value, uint32 numValues );

            /** Returns the value. Assumes this is a manual value.
                Assumes the param holds enough bytes to fill the type
                of value you want to retrieve.
                Examples:
                    uint32 myVal;
                    param->getManualValue( myVal );
                    Vector4 myVector4;
                    param->getManualValue( myVector4 );
            */
            template <typename T>
            void getManualValue( T &value ) const
            {
                assert( !isAutomatic && !isEx && sizeof( T ) <= mp.dataSizeBytes );
                memcpy( &value, mp.dataBytes, sizeof( T ) );
            }
            /// See other overload.
            /// Examples:
            ///     uint32 myVal = param->getManualValue();
            ///     Vector4 myVector4 = param->getManualValue();
            template <typename T>
            T getManualValue() const
            {
                T retVal;
                getManualValue( retVal );
                return retVal;
            }

            template <typename T>
            void setManualValueEx( const T *value, uint32 numValues, ElementType elementType );

            /** Shortcut for setting an array with a custom pointer. We won't free it.
                Caller is responsible for keeping the pointer alive while
                this Params hold it.
            @remarks
                This version is meant to be used when setManualValue's limit of 16 values
                is not enough for you.

                Remember to call @see ShaderParams::setDirty otherwise
                changes won't take effect.
            */
            void setManualValueEx( const float *value, uint32 numValues );
            void setManualValueEx( const int32 *value, uint32 numValues );
            void setManualValueEx( const uint32 *value, uint32 numValues );
        };

        typedef vector<Param>::type ParamVec;

        /// Don't log exceptions about missing parameters
        bool     mSilenceMissingParameterWarnings;
        uint32   mUpdateCounter;
        ParamVec mParams;

        ShaderParams();

        void updateParameters( GpuProgramParametersSharedPtr params, bool bForce );

        /// Call this whenever you've updated a parameter in mParams
        void   setDirty() { ++mUpdateCounter; }
        uint32 getUpdateCounter() const { return mUpdateCounter; }

        /// Finds a parameter. Returns null if not found.
        /// This operation is O(N) as it makes a linear search.
        /// You can cache the return pointer (as long as the
        /// iterators from mParams aren't invalidated)
        Param *findParameter( const String &name );

        /** Removes a parameter if it matches the name
        @remarks
            You shouldn't have multiple parameters with the same name. But if you do,
            then we will only remove the first match.
            You can call this function again until all matches are removed
            (beware of O(N^2) behavior though!)

            Calling this function may alter the order of entries in mParams!
        @param name
        @return
            Returns true if we removed a parameter with such name.
            Returns false if said parameter couldn't be found.
        */
        bool removeParameterNoThrow( const String &name );
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
