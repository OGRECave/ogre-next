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

#ifndef _OgreLightProfiles_H_
#define _OgreLightProfiles_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "LightProfiles/OgreIesLoader.h"
#include "OgreIdString.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /**
    @class LightProfiles
    */
    class _OgreHlmsPbsExport LightProfiles : public UtilityAlloc
    {
        FastArray<IesLoader *> mIesData;
        map<IdString, size_t>::type mIesNameMap;
        TextureGpu *mLightProfilesTexture;

        HlmsPbs *mHlmsPbs;
        TextureGpuManager *mTextureGpuManager;

        void destroyTexture( void );

    public:
        LightProfiles( HlmsPbs *hlmsPbs, TextureGpuManager *textureGpuManager );
        ~LightProfiles();

        /// Loads an IES (Illuminating Engineering Society) profile to memory
        ///
        /// When trying to load an IES file that is already loaded,
        /// if throwOnDuplicate = false then this function does nothing
        void loadIesProfile( const String &filename, const String &resourceGroup,
                             bool throwOnDuplicate = true );

        /** After you're done with all your loadIesProfile calls, call this function
            to generate the texture required for rendering.
        @remarks
            You can add more profiles later by calling loadIesProfile, but you must call
            build again afterwards

            You can call LightProfiles::assignProfile before calling build
        */
        void build( void );

        /** Assigns the given profile to the light.
            Use either:
            @code
                lightProfile->assignProfile( IdString(), light );
                lightProfile->assignProfile( IdString( "" ), light );
            @endcode
            To unset any profile
        @remarks
            Throws if no profile with the given name exists, and profileName is not IdString()
        @param profileName
            Name of the profile to assign to the light.
            Use IdString() or IdString( "" ) to unset any profile the light might already have
        @param light
            Light to set the profile to
        */
        void assignProfile( IdString profileName, Light *light );

        /// Returns the profile associated with the name. Nullptr if not found
        const IesLoader *getProfile( IdString profileName ) const;
        /// Returns the profile associated with the light. Nullptr if none
        /// Assumes the light's profile was created by 'this'
        const IesLoader *getProfile( Light *light ) const;
        /// Returns the name of the profile associated with the light. Empty if none
        /// Assumes the light's profile was created by 'this'
        const String &getProfileName( Light *light ) const;
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
