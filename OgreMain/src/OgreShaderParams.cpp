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

#include "OgreShaderParams.h"

#include "OgreGpuProgram.h"
#include "OgreLogManager.h"
#include "OgreMatrix4.h"
#include "OgreVector2.h"
#include "OgreVector3.h"

namespace Ogre
{
    ShaderParams::ShaderParams() : mSilenceMissingParameterWarnings( false ), mUpdateCounter( 0 ) {}
    //-----------------------------------------------------------------------------------
    void ShaderParams::updateParameters( GpuProgramParametersSharedPtr params, bool bForce )
    {
        ParamVec::iterator itor = mParams.begin();
        ParamVec::iterator endt = mParams.end();

        while( itor != endt )
        {
            if( itor->isDirty || bForce )
            {
                itor->isDirty = false;
                try
                {
                    if( !itor->isAutomatic )
                    {
                        if( !itor->isEx )
                        {
                            switch( itor->mp.elementType )
                            {
                            case ElementInt:
                                params->setNamedConstant(
                                    itor->name, reinterpret_cast<const int *>( itor->mp.dataBytes ),
                                    itor->mp.dataSizeBytes / sizeof( int ), 1 );
                                break;
                            case ElementUInt:
                                params->setNamedConstant(
                                    itor->name, reinterpret_cast<const uint *>( itor->mp.dataBytes ),
                                    itor->mp.dataSizeBytes / sizeof( uint ), 1 );
                                break;
                            case ElementFloat:
                                params->setNamedConstant(
                                    itor->name, reinterpret_cast<const float *>( itor->mp.dataBytes ),
                                    itor->mp.dataSizeBytes / sizeof( float ), 1 );
                                break;
                            }
                        }
                        else
                        {
                            switch( itor->mpEx.elementType )
                            {
                            case ElementInt:
                                params->setNamedConstant(
                                    itor->name, reinterpret_cast<const int *>( itor->mpEx.dataBytes ),
                                    itor->mpEx.dataSizeBytes / sizeof( int ), 1 );
                                break;
                            case ElementUInt:
                                params->setNamedConstant(
                                    itor->name, reinterpret_cast<const uint *>( itor->mpEx.dataBytes ),
                                    itor->mpEx.dataSizeBytes / sizeof( uint ), 1 );
                                break;
                            case ElementFloat:
                                params->setNamedConstant(
                                    itor->name, reinterpret_cast<const float *>( itor->mpEx.dataBytes ),
                                    itor->mpEx.dataSizeBytes / sizeof( float ), 1 );
                                break;
                            }
                        }
                    }
                    else
                    {
                        if( itor->ap.extraParamType != ElementFloat )
                        {
                            params->setNamedAutoConstant(
                                itor->name, itor->ap.acType,
                                static_cast<size_t>( itor->ap.extraParamValue ) );
                        }
                        else
                        {
                            params->setNamedAutoConstantReal(
                                itor->name, itor->ap.acType,
                                static_cast<Real>( itor->ap.extraParamValue ) );
                        }
                    }
                }
                catch( Exception &e )
                {
                    if( e.getNumber() != Exception::ERR_ITEM_NOT_FOUND )
                        throw;
                    else if( !mSilenceMissingParameterWarnings )
                    {
                        LogManager::getSingleton().logMessage( e.getFullDescription() );
                    }
                }
            }

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    ShaderParams::Param *ShaderParams::findParameter( const String &name )
    {
        ShaderParams::Param *retVal = 0;

        ParamVec::iterator itor = mParams.begin();
        ParamVec::iterator endt = mParams.end();

        while( itor != endt && itor->name != name )
            ++itor;

        if( itor != endt )
            retVal = &( *itor );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    bool ShaderParams::removeParameterNoThrow( const String &name )
    {
        bool wasRemoved = false;

        ParamVec::iterator itor = mParams.begin();
        ParamVec::iterator endt = mParams.end();

        while( itor != endt && itor->name != name )
            ++itor;

        if( itor != endt )
        {
            efficientVectorRemove( mParams, itor );
            wasRemoved = true;
        }

        return wasRemoved;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValue( const Vector2 &value )
    {
        this->isAutomatic = false;
        this->isEx = false;
        this->isDirty = true;
        mp.elementType = ElementFloat;
        mp.dataSizeBytes = sizeof( float ) * 2;
#if !OGRE_DOUBLE_PRECISION
        memcpy( mp.dataBytes, value.ptr(), sizeof( float ) * 2 );
#else
        for( int i = 0; i < 2; ++i )
            reinterpret_cast<float *>( mp.dataBytes )[i] = static_cast<float>( value[i] );
#endif
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValue( const Vector3 &value )
    {
        this->isAutomatic = false;
        this->isEx = false;
        this->isDirty = true;
        mp.elementType = ElementFloat;
        mp.dataSizeBytes = sizeof( float ) * 3;
#if !OGRE_DOUBLE_PRECISION
        memcpy( mp.dataBytes, value.ptr(), sizeof( float ) * 3 );
#else
        for( int i = 0; i < 3; ++i )
            reinterpret_cast<float *>( mp.dataBytes )[i] = static_cast<float>( value[i] );
#endif
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValue( const Vector4 &value )
    {
        this->isAutomatic = false;
        this->isEx = false;
        this->isDirty = true;
        mp.elementType = ElementFloat;
        mp.dataSizeBytes = sizeof( float ) * 4;
#if !OGRE_DOUBLE_PRECISION
        memcpy( mp.dataBytes, value.ptr(), sizeof( float ) * 4 );
#else
        for( int i = 0; i < 4; ++i )
            reinterpret_cast<float *>( mp.dataBytes )[i] = static_cast<float>( value[i] );
#endif
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValue( const Matrix3 &_value )
    {
        this->isAutomatic = false;
        this->isEx = false;
        this->isDirty = true;
        mp.elementType = ElementFloat;
        mp.dataSizeBytes = sizeof( float ) * 9u;
        const Real *RESTRICT_ALIAS value = reinterpret_cast<const Real * RESTRICT_ALIAS>( &_value );
        float *RESTRICT_ALIAS dstValue = reinterpret_cast<float * RESTRICT_ALIAS>( mp.dataBytes );

        for( int i = 0; i < 9; ++i )
            dstValue[i] = static_cast<float>( value[i] );
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValue( const Matrix4 &_value )
    {
        this->isAutomatic = false;
        this->isEx = false;
        this->isDirty = true;
        mp.elementType = ElementFloat;
        mp.dataSizeBytes = sizeof( float ) * 16u;

        const Real *RESTRICT_ALIAS value = reinterpret_cast<const Real * RESTRICT_ALIAS>( &_value );
#if !OGRE_DOUBLE_PRECISION
        memcpy( mp.dataBytes, value, sizeof( float ) * 16u );
#else
        for( int i = 0; i < 16; ++i )
            reinterpret_cast<float *>( mp.dataBytes )[i] = static_cast<float>( value[i] );
#endif
    }
    //-----------------------------------------------------------------------------------
    template <typename T>
    void ShaderParams::Param::setManualValue( T value, uint32 numValues, ElementType elementType )
    {
        this->isAutomatic = false;
        this->isEx = false;
        this->isDirty = true;
        mp.elementType = elementType;
        mp.dataSizeBytes = static_cast<uint8>( std::min( 4u * numValues, 64u ) );  // Don't overflow
        memcpy( mp.dataBytes, value, mp.dataSizeBytes );
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValue( float value )
    {
        setManualValue( &value, 1u, ElementFloat );
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValue( const float *value, uint32 numValues )
    {
        setManualValue( value, numValues, ElementFloat );
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValue( int32 value ) { setManualValue( &value, 1u, ElementInt ); }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValue( const int32 *value, uint32 numValues )
    {
        setManualValue( value, numValues, ElementInt );
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValue( uint32 value )
    {
        setManualValue( &value, 1u, ElementUInt );
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValue( const uint32 *value, uint32 numValues )
    {
        setManualValue( value, numValues, ElementUInt );
    }
    //-----------------------------------------------------------------------------------
    template <typename T>
    void ShaderParams::Param::setManualValueEx( const T *value, uint32 numValues,
                                                ElementType elementType )
    {
        this->isAutomatic = false;
        this->isEx = true;
        this->isDirty = true;
        mpEx.elementType = elementType;
        mpEx.dataSizeBytes = static_cast<uint16>( std::min<uint32>(
            sizeof( T ) * numValues, std::numeric_limits<uint16>::max() ) );  // Don't overflow
        mpEx.dataBytes = value;
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValueEx( const float *value, uint32 numValues )
    {
        setManualValueEx( value, numValues, ElementFloat );
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValueEx( const int32 *value, uint32 numValues )
    {
        setManualValueEx( value, numValues, ElementInt );
    }
    //-----------------------------------------------------------------------------------
    void ShaderParams::Param::setManualValueEx( const uint32 *value, uint32 numValues )
    {
        setManualValueEx( value, numValues, ElementUInt );
    }
}  // namespace Ogre
