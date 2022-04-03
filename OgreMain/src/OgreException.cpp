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

#include "OgreException.h"

#include "OgreLogManager.h"

#include <sstream>

#ifdef __BORLANDC__
#    include <stdio.h>
#endif

namespace Ogre
{
    Exception::Exception( int num, const String &desc, const String &src ) :
        line( 0 ),
        number( num ),
        description( desc ),
        source( src )
    {
        // Log this error - not any more, allow catchers to do it
        // LogManager::getSingleton().logMessage(this->getFullDescription());
    }

    Exception::Exception( int num, const String &desc, const String &src, const char *typ,
                          const char *fil, long lin ) :
        line( lin ),
        number( num ),
        typeName( typ ),
        description( desc ),
        source( src ),
        file( fil )
    {
        // Log this error, mask it from debug though since it may be caught and ignored
        if( LogManager::getSingletonPtr() )
        {
            LogManager::getSingleton().logMessage( this->getFullDescription(), LML_CRITICAL, true );
        }
    }

    Exception::Exception( const Exception &rhs ) :
        line( rhs.line ),
        number( rhs.number ),
        typeName( rhs.typeName ),
        description( rhs.description ),
        source( rhs.source ),
        file( rhs.file )
    {
    }

    Exception::~Exception() noexcept {}

    Exception &Exception::operator=( const Exception &rhs )
    {
        description = rhs.description;
        number = rhs.number;
        source = rhs.source;
        file = rhs.file;
        line = rhs.line;
        typeName = rhs.typeName;

        return *this;
    }

    const String &Exception::getFullDescription() const
    {
        if( fullDesc.empty() )
        {
            StringStream desc;

            desc << "OGRE EXCEPTION(" << number << ":" << typeName << "): " << description << " in "
                 << source;

            if( line > 0 )
            {
                desc << " at " << file << " (line " << line << ")";
            }

            fullDesc = desc.str();
        }

        return fullDesc;
    }

    int Exception::getNumber() const throw() { return number; }

    UnimplementedException::UnimplementedException( int inNumber, const String &inDescription,
                                                    const String &inSource, const char *inFile,
                                                    long inLine ) :
        Exception( inNumber, inDescription, inSource, "UnimplementedException", inFile, inLine )
    {
    }

    UnimplementedException::~UnimplementedException() throw() {}

    FileNotFoundException::FileNotFoundException( int inNumber, const String &inDescription,
                                                  const String &inSource, const char *inFile,
                                                  long inLine ) :
        Exception( inNumber, inDescription, inSource, "FileNotFoundException", inFile, inLine )
    {
    }

    FileNotFoundException::~FileNotFoundException() throw() {}

    IOException::IOException( int inNumber, const String &inDescription, const String &inSource,
                              const char *inFile, long inLine ) :
        Exception( inNumber, inDescription, inSource, "IOException", inFile, inLine )
    {
    }

    IOException::~IOException() throw() {}

    InvalidStateException::InvalidStateException( int inNumber, const String &inDescription,
                                                  const String &inSource, const char *inFile,
                                                  long inLine ) :
        Exception( inNumber, inDescription, inSource, "InvalidStateException", inFile, inLine )
    {
    }

    InvalidStateException::~InvalidStateException() throw() {}

    InvalidParametersException::InvalidParametersException( int inNumber, const String &inDescription,
                                                            const String &inSource, const char *inFile,
                                                            long inLine ) :
        Exception( inNumber, inDescription, inSource, "InvalidParametersException", inFile, inLine )
    {
    }

    InvalidParametersException::~InvalidParametersException() throw() {}

    ItemIdentityException::ItemIdentityException( int inNumber, const String &inDescription,
                                                  const String &inSource, const char *inFile,
                                                  long inLine ) :
        Exception( inNumber, inDescription, inSource, "ItemIdentityException", inFile, inLine )
    {
    }

    ItemIdentityException::~ItemIdentityException() throw() {}

    InternalErrorException::InternalErrorException( int inNumber, const String &inDescription,
                                                    const String &inSource, const char *inFile,
                                                    long inLine ) :
        Exception( inNumber, inDescription, inSource, "InternalErrorException", inFile, inLine )
    {
    }

    InternalErrorException::~InternalErrorException() throw() {}

    RenderingAPIException::RenderingAPIException( int inNumber, const String &inDescription,
                                                  const String &inSource, const char *inFile,
                                                  long inLine ) :
        Exception( inNumber, inDescription, inSource, "RenderingAPIException", inFile, inLine )
    {
    }

    RenderingAPIException::~RenderingAPIException() throw() {}

    RuntimeAssertionException::RuntimeAssertionException( int inNumber, const String &inDescription,
                                                          const String &inSource, const char *inFile,
                                                          long inLine ) :
        Exception( inNumber, inDescription, inSource, "RuntimeAssertionException", inFile, inLine )
    {
    }

    RuntimeAssertionException::~RuntimeAssertionException() throw() {}

    InvalidCallException::InvalidCallException( int inNumber, const String &inDescription,
                                                const String &inSource, const char *inFile,
                                                long inLine ) :
        Exception( inNumber, inDescription, inSource, "InvalidCallException", inFile, inLine )
    {
    }

    InvalidCallException::~InvalidCallException() throw() {}
}  // namespace Ogre
