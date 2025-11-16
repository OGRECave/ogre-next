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
#ifndef __GL3PlusPrerequisites_H__
#define __GL3PlusPrerequisites_H__

#include "OgrePrerequisites.h"

#include "OgreLogManager.h"

#include "OgreGL3PlusBuildSettings.h"

namespace Ogre
{
    // Forward declarations
    class GL3PlusAsyncTextureTicket;
    class GL3PlusDynamicBuffer;
    class GL3PlusStagingBuffer;
    class GL3PlusStagingTexture;
    class GL3PlusSupport;
    class GL3PlusRenderSystem;
    class GL3PlusContext;
    struct GL3PlusHlmsPso;
    class GL3PlusTextureGpuManager;
    class GL3PlusVaoManager;

    class GLSLShader;

    typedef SharedPtr<GLSLShader> GLSLShaderPtr;
}  // namespace Ogre

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#    if !defined( __MINGW32__ )
#        define WIN32_LEAN_AND_MEAN
#        ifndef NOMINMAX
#            define NOMINMAX  // required to stop windows.h messing up std::min
#        endif
#    endif
// #   define WGL_WGLEXT_PROTOTYPES
#    include <GL/gl3w.h>
#    include <GL/glext.h>
#    include <GL/wglext.h>
#    include <windows.h>
#    include <wingdi.h>
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
#    include <GL/gl3w.h>
#    include <GL/glext.h>
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#    include <GL/gl3w.h>
#    include <GL/glext.h>
#    include <OpenGL/OpenGLAvailability.h>
#    include <OpenGL/gl3ext.h>
#endif

// Lots of generated code in here which triggers the new VC CRT security warnings
#if !defined( _CRT_SECURE_NO_DEPRECATE )
#    define _CRT_SECURE_NO_DEPRECATE
#endif

#if( OGRE_PLATFORM == OGRE_PLATFORM_WIN32 ) && !defined( __MINGW32__ ) && !defined( OGRE_STATIC_LIB )
#    ifdef RenderSystem_GL3Plus_EXPORTS
#        define _OgreGL3PlusExport __declspec( dllexport )
#    else
#        if defined( __MINGW32__ )
#            define _OgreGL3PlusExport
#        else
#            define _OgreGL3PlusExport __declspec( dllimport )
#        endif
#    endif
#elif defined( OGRE_GCC_VISIBILITY )
#    if !defined( OGRE_STATIC_LIB )
#        define _OgreGL3PlusExport __attribute__( ( visibility( "default" ) ) )
#    else
#        define _OgreGL3PlusExport __attribute__( ( visibility( "hidden" ) ) )
#    endif
#else
#    define _OgreGL3PlusExport
#endif

// Convenience macro from ARB_vertex_buffer_object spec
#define GL_BUFFER_OFFSET( i ) reinterpret_cast<void *>( ( i ) )

#define ENABLE_GL_CHECK 0
#if ENABLE_GL_CHECK
#    include "OgreStringVector.h"
#    define OGRE_CHECK_GL_ERROR( glFunc ) \
        { \
            glFunc; \
            int e = glGetError(); \
            if( e != 0 ) \
            { \
                const char *errorString = ""; \
                switch( e ) \
                { \
                case GL_INVALID_ENUM: \
                    errorString = "GL_INVALID_ENUM"; \
                    break; \
                case GL_INVALID_VALUE: \
                    errorString = "GL_INVALID_VALUE"; \
                    break; \
                case GL_INVALID_OPERATION: \
                    errorString = "GL_INVALID_OPERATION"; \
                    break; \
                case GL_INVALID_FRAMEBUFFER_OPERATION: \
                    errorString = "GL_INVALID_FRAMEBUFFER_OPERATION"; \
                    break; \
                case GL_OUT_OF_MEMORY: \
                    errorString = "GL_OUT_OF_MEMORY"; \
                    break; \
                default: \
                    break; \
                } \
                char         msgBuf[4096]; \
                StringVector tokens = StringUtil::split( #glFunc, "(" ); \
                sprintf( msgBuf, "OpenGL error 0x%04X %s in %s at line %i for %s\n", e, errorString, \
                         OGRE_CURRENT_FUNCTION, __LINE__, tokens[0].c_str() ); \
                LogManager::getSingleton().logMessage( msgBuf, LML_CRITICAL ); \
            } \
        }

#    define EGL_CHECK_ERROR \
        { \
            int e = eglGetError(); \
            if( ( e != 0 ) && ( e != EGL_SUCCESS ) ) \
            { \
                char msgBuf[4096]; \
                sprintf( msgBuf, "EGL error 0x%04X in %s at line %i\n", e, OGRE_CURRENT_FUNCTION, \
                         __LINE__ ); \
                LogManager::getSingleton().logMessage( msgBuf ); \
                OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, msgBuf, OGRE_CURRENT_FUNCTION ); \
            } \
        }
#else
#    define OGRE_CHECK_GL_ERROR( glFunc ) \
        do \
        { \
            glFunc; \
        } while( 0 )
#    define EGL_CHECK_ERROR \
        { \
        }
#endif

#define OCGE OGRE_CHECK_GL_ERROR

namespace Ogre
{
    extern void ogreGlObjectLabel( GLenum identifier, GLuint name, GLsizei length, const GLchar *label );
    extern void ogreGlObjectLabel( GLenum identifier, GLuint name, const String &label );
}  // namespace Ogre

#endif  // #ifndef __GL3PlusPrerequisites_H__
