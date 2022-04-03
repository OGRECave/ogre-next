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
#ifndef __ParticleEmitterCommands_H__
#define __ParticleEmitterCommands_H__

#include "OgrePrerequisites.h"
#include "OgreStringInterface.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Effects
     *  @{
     */

    namespace EmitterCommands
    {
        /// Command object for ParticleEmitter  - see ParamCommand
        class _OgreExport CmdAngle final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdColour final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdColourRangeStart final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdColourRangeEnd final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdDirection final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdUp final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdDirPositionRef final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdEmissionRate final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdVelocity final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdMinVelocity final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdMaxVelocity final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdTTL final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdMinTTL final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdMaxTTL final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdPosition final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdDuration final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdMinDuration final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdMaxDuration final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdRepeatDelay final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdMinRepeatDelay final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdMaxRepeatDelay final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };
        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdName final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

        /// Command object for particle emitter  - see ParamCommand
        class _OgreExport CmdEmittedEmitter final : public ParamCommand
        {
        public:
            String doGet( const void *target ) const override;
            void   doSet( void *target, const String &val ) override;
        };

    }  // namespace EmitterCommands
    /** @} */
    /** @} */

}  // namespace Ogre

#endif
