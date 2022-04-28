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

#ifndef _D3D11HARWAREOCCLUSIONQUERY_H__
#define _D3D11HARWAREOCCLUSIONQUERY_H__

#include "OgreD3D11Prerequisites.h"

#include "OgreD3D11Device.h"
#include "OgreHardwareOcclusionQuery.h"

namespace Ogre
{
    // If you use multiple rendering passes you can test only the first pass and all other passes don't
    // have to be rendered if the first pass results has too few pixels visible.

    // Be sure to render all occluder first and what's out so the RenderQue don't switch places on
    // the occluding objects and the tested objects because it thinks it's more effective..

    /**
     * This is a class that is the DirectX9 implementation of
     * hardware occlusion testing.
     *
     * @author Lee Sandberg, email lee@abcmedia.se
     *
     * Updated on 12/7/2004 by Chris McGuirk
     * Updated on 4/8/2005 by Tuan Kuranes email: tuan.kuranes@free.fr
     */
    class _OgreD3D11Export D3D11HardwareOcclusionQuery final : public HardwareOcclusionQuery
    {
        //----------------------------------------------------------------------
        // Public methods
        //--
    public:
        /**
         * Default object constructor
         *
         */
        D3D11HardwareOcclusionQuery( D3D11Device &device );

        /**
         * Object destructor
         */
        ~D3D11HardwareOcclusionQuery() override;

        //------------------------------------------------------------------
        // Occlusion query functions (see base class documentation for this)
        //--

        void beginOcclusionQuery() override;
        void endOcclusionQuery() override;

        bool pullOcclusionQuery( unsigned int *NumOfFragments ) override;

        unsigned int getLastQuerysPixelcount() { return mPixelCount; }
        bool         isStillOutstanding() override;

        //----------------------------------------------------------------------
        // private members
        //--
    private:
        ComPtr<ID3D11Query> mQuery;
        D3D11Device        &mDevice;
    };

}  // namespace Ogre

#endif
