/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#ifndef _OgreWorkarounds_H_
#define _OgreWorkarounds_H_

#ifdef OGRE_BUILD_RENDERSYSTEM_GLES2

//PowerVR SGX 540 does not correctly transpose matrices in glProgramUniformMatrix4fvEXT,
//so we have to do that manually (which is inconsistent with the way GL3+ works on desktop)
//
//First seen: 2010-02-25 (probably, at least)
//Last seen:  2014-05-30 on Samsung Galaxy Tab 2 (GT-P3110);
//		Android 4.2.2 (Samsung officially discontinued updates for this device)
//Additional Info:
//	https://twitter.com/KeepItFoolish/status/472456232738234368
#define OGRE_GLES2_WORKAROUND_1		1

//For some reason cubemapping in ES, the vector needs to be rotated 90° around the X axis.
//May be it's a difference in the ES vs GL spec, or an Ogre bug when uploading the faces.
//
//First seen: ???
//Last seen:  2014-08-19 on All devices
#define OGRE_GLES2_WORKAROUND_2		1

#endif

#endif