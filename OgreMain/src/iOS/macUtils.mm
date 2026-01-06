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

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include "OgreLogManager.h"
#include "OgreString.h"
#include "macUtils.h"

namespace Ogre
{
    // Basically a dummy function.  Dynamic libraries aren't supported on iOS
    void *mac_loadDylib( const char *name ) { return NULL; }

    String macBundlePath()
    {
        char path[PATH_MAX];
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        assert( mainBundle );

        CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle );
        assert( mainBundleURL );

        CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle );
        assert( cfStringRef );

        CFStringGetFileSystemRepresentation( cfStringRef, path, PATH_MAX );

        CFRelease( mainBundleURL );
        CFRelease( cfStringRef );

        return String( path );
    }

    String iOSDocumentsDirectory()
    {
        NSArray *paths =
            NSSearchPathForDirectoriesInDomains( NSDocumentDirectory, NSUserDomainMask, YES );
        NSString *documentsDirectory = [paths objectAtIndex:0];

        return String( [documentsDirectory fileSystemRepresentation] );
    }

    String macCachePath( bool bAutoCreate )
    {
        NSURL *cachesURL = [NSFileManager.defaultManager URLForDirectory:NSCachesDirectory
                                                                inDomain:NSUserDomainMask
                                                       appropriateForURL:nil
                                                                  create:YES
                                                                   error:nil];
        NSURL *myDirURL = cachesURL;

        if( NSBundle.mainBundle.bundleIdentifier )
        {
            // May be nullptr if bundle is not correctly set (e.g. samples)
            myDirURL = [cachesURL URLByAppendingPathComponent:NSBundle.mainBundle.bundleIdentifier
                                                  isDirectory:YES];
        }
        else
        {
            LogManager::getSingleton().logMessage( "WARNING: NS Bundle Identifier not set!",
                                                   LML_CRITICAL );
        }

        if( bAutoCreate )
        {
            [NSFileManager.defaultManager createDirectoryAtURL:myDirURL
                                   withIntermediateDirectories:YES
                                                    attributes:nil
                                                         error:nil];
        }
        return myDirURL.fileSystemRepresentation;
    }

    String macTempFileName()
    {
        NSString *tempFilePath;
        NSFileManager *fileManager = [NSFileManager defaultManager];
        for( ;; )
        {
            NSString *baseName = [NSString stringWithFormat:@"tmp-%x", arc4random()];
            tempFilePath = [NSTemporaryDirectory() stringByAppendingPathComponent:baseName];
            if( ![fileManager fileExistsAtPath:tempFilePath] )
                break;
        }
        return String( [tempFilePath fileSystemRepresentation] );
    }

}  // namespace Ogre
