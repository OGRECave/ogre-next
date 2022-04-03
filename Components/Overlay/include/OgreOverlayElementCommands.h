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
#ifndef __OverlayElementEmitterCommands_H__
#define __OverlayElementEmitterCommands_H__

#include "OgreOverlayPrerequisites.h"

#include "OgreStringInterface.h"

namespace Ogre
{
    namespace v1
    {
        /** \addtogroup Core
         *  @{
         */
        /** \addtogroup Overlays
         *  @{
         */

        namespace OverlayElementCommands
        {
            /// Command object for OverlayElement  - see ParamCommand
            class _OgreOverlayExport CmdLeft final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };
            /// Command object for OverlayElement  - see ParamCommand
            class _OgreOverlayExport CmdTop final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };
            /// Command object for OverlayElement  - see ParamCommand
            class _OgreOverlayExport CmdWidth final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };
            /// Command object for OverlayElement  - see ParamCommand
            class _OgreOverlayExport CmdHeight final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };
            /// Command object for OverlayElement  - see ParamCommand
            class _OgreOverlayExport CmdMaterial final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };
            /// Command object for OverlayElement  - see ParamCommand
            class _OgreOverlayExport CmdCaption final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };
            /// Command object for OverlayElement  - see ParamCommand
            class _OgreOverlayExport CmdMetricsMode final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };
            /// Command object for OverlayElement  - see ParamCommand
            class _OgreOverlayExport CmdHorizontalAlign final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };
            /// Command object for OverlayElement  - see ParamCommand
            class _OgreOverlayExport CmdVerticalAlign final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };
            /// Command object for OverlayElement  - see ParamCommand
            class _OgreOverlayExport CmdVisible final : public ParamCommand
            {
            public:
                String doGet( const void *target ) const override;
                void   doSet( void *target, const String &val ) override;
            };

        }  // namespace OverlayElementCommands
        /** @} */
        /** @} */
    }  // namespace v1
}  // namespace Ogre

#endif
