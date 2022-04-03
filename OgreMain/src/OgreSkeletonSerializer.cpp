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

#include "OgreSkeletonSerializer.h"

#include "OgreAnimation.h"
#include "OgreAnimationTrack.h"
#include "OgreDataStream.h"
#include "OgreKeyFrame.h"
#include "OgreLogManager.h"
#include "OgreOldBone.h"
#include "OgreSkeleton.h"
#include "OgreSkeletonFileFormat.h"

#include <fstream>
#include <sstream>

namespace Ogre
{
    namespace v1
    {
        /// stream overhead = ID + size
        const long SSTREAM_OVERHEAD_SIZE = sizeof( uint16 ) + sizeof( uint32 );
        const uint16 HEADER_STREAM_ID_EXT = 0x1000;
        //---------------------------------------------------------------------
        SkeletonSerializer::SkeletonSerializer()
        {
            // Version number
            // NB changed to include bone names in 1.1
            mVersion = "[Unknown]";
        }
        //---------------------------------------------------------------------
        SkeletonSerializer::~SkeletonSerializer() {}

        //---------------------------------------------------------------------
        void SkeletonSerializer::exportSkeleton( const Skeleton *pSkeleton, const String &filename,
                                                 SkeletonVersion ver, Endian endianMode )
        {
            std::fstream *f = OGRE_NEW_T( std::fstream, MEMCATEGORY_GENERAL )();
            f->open( filename.c_str(), std::ios::binary | std::ios::out );
            DataStreamPtr stream( OGRE_NEW FileStreamDataStream( f ) );

            exportSkeleton( pSkeleton, stream, ver, endianMode );

            stream->close();
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::exportSkeleton( const Skeleton *pSkeleton, DataStreamPtr stream,
                                                 SkeletonVersion ver, Endian endianMode )
        {
            setWorkingVersion( ver );
            // Decide on endian mode
            determineEndianness( endianMode );

            mStream = stream;
            if( !stream->isWriteable() )
            {
                OGRE_EXCEPT( Exception::ERR_CANNOT_WRITE_TO_FILE,
                             "Unable to write to stream " + stream->getName(),
                             "SkeletonSerializer::exportSkeleton" );
            }

            writeFileHeader();

            pushInnerChunk( mStream );
            // Write main skeleton data
            LogManager::getSingleton().logMessage( "Exporting bones.." );
            writeSkeleton( pSkeleton, ver );
            LogManager::getSingleton().logMessage( "Bones exported." );

            // Write all animations
            unsigned short numAnims = pSkeleton->getNumAnimations();
            LogManager::getSingleton().stream() << "Exporting animations, count=" << numAnims;
            for( unsigned short i = 0; i < numAnims; ++i )
            {
                Animation *pAnim = pSkeleton->getAnimation( i );
                LogManager::getSingleton().stream() << "Exporting animation: " << pAnim->getName();
                writeAnimation( pSkeleton, pAnim, ver );
                LogManager::getSingleton().logMessage( "Animation exported." );
            }

            // Write links
            Skeleton::LinkedSkeletonAnimSourceIterator linkIt =
                pSkeleton->getLinkedSkeletonAnimationSourceIterator();
            while( linkIt.hasMoreElements() )
            {
                const LinkedSkeletonAnimationSource &link = linkIt.getNext();
                writeSkeletonAnimationLink( pSkeleton, link );
            }
            popInnerChunk( stream );
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::importSkeleton( DataStreamPtr &stream, Skeleton *pSkel )
        {
            // Determine endianness (must be the first thing we do!)
            determineEndianness( stream );

            // Check header
            readFileHeader( stream );
            pushInnerChunk( stream );
            unsigned short streamID = readChunk( stream );

            while( !stream->eof() )
            {
                switch( streamID )
                {
                case SKELETON_BLENDMODE:
                {
                    // Optional blend mode
                    uint16 blendMode;
                    readShorts( stream, &blendMode, 1 );
                    pSkel->setBlendMode( static_cast<SkeletonAnimationBlendMode>( blendMode ) );
                    break;
                }
                case SKELETON_BONE:
                    readBone( stream, pSkel );
                    break;
                case SKELETON_BONE_PARENT:
                    readBoneParent( stream, pSkel );
                    break;
                case SKELETON_ANIMATION:
                    readAnimation( stream, pSkel );
                    break;
                case SKELETON_ANIMATION_LINK:
                    readSkeletonAnimationLink( stream, pSkel );
                    break;
                default:
                    break;
                }

                streamID = readChunk( stream );
            }
            // Assume bones are stored in the binding pose
            pSkel->setBindingPose();
            popInnerChunk( stream );
        }

        //---------------------------------------------------------------------
        void SkeletonSerializer::setWorkingVersion( SkeletonVersion ver )
        {
            if( ver == SKELETON_VERSION_1_0 )
                mVersion = "[Serializer_v1.10]";
            else
                mVersion = "[Serializer_v1.80]";
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::writeSkeleton( const Skeleton *pSkel, SkeletonVersion ver )
        {
            // Write blend mode
            if( (int)ver > (int)SKELETON_VERSION_1_0 )
            {
                writeChunkHeader( SKELETON_BLENDMODE, SSTREAM_OVERHEAD_SIZE + sizeof( unsigned short ) );
                uint16 blendMode = static_cast<uint16>( pSkel->getBlendMode() );
                writeShorts( &blendMode, 1 );
            }

            // Write each bone
            unsigned short numBones = pSkel->getNumBones();
            unsigned short i;
            for( i = 0; i < numBones; ++i )
            {
                OldBone *pBone = pSkel->getBone( i );
                writeBone( pSkel, pBone );
            }
            // Write parents
            for( i = 0; i < numBones; ++i )
            {
                OldBone *pBone = pSkel->getBone( i );
                unsigned short handle = pBone->getHandle();
                OldBone *pParent = (OldBone *)pBone->getParent();
                if( pParent != NULL )
                {
                    writeBoneParent( pSkel, handle, pParent->getHandle() );
                }
            }
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::writeBone( const Skeleton *pSkel, const OldBone *pBone )
        {
            writeChunkHeader( SKELETON_BONE, calcBoneSize( pSkel, pBone ) );

            unsigned short handle = pBone->getHandle();
            // char* name
            writeString( pBone->getName() );
#if OGRE_SERIALIZER_VALIDATE_CHUNKSIZE
            // Hack to fix chunk size validation:
            mChunkSizeStack.back() += calcStringSize( pBone->getName() );
#endif
            // unsigned short handle            : handle of the bone, should be contiguous & start at 0
            writeShorts( &handle, 1 );
            // Vector3 position                 : position of this bone relative to parent
            writeObject( pBone->getPosition() );
            // Quaternion orientation           : orientation of this bone relative to parent
            writeObject( pBone->getOrientation() );
            // Vector3 scale                    : scale of this bone relative to parent
            if( pBone->getScale() != Vector3::UNIT_SCALE )
            {
                writeObject( pBone->getScale() );
            }
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::writeBoneParent( const Skeleton *pSkel, unsigned short boneId,
                                                  unsigned short parentId )
        {
            writeChunkHeader( SKELETON_BONE_PARENT, calcBoneParentSize( pSkel ) );

            // unsigned short handle             : child bone
            writeShorts( &boneId, 1 );
            // unsigned short parentHandle   : parent bone
            writeShorts( &parentId, 1 );
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::writeAnimation( const Skeleton *pSkel, const Animation *anim,
                                                 SkeletonVersion ver )
        {
            writeChunkHeader( SKELETON_ANIMATION, calcAnimationSize( pSkel, anim, ver ) );

            // char* name                       : Name of the animation
            writeString( anim->getName() );
            // float length                      : Length of the animation in seconds
            float len = anim->getLength();
            writeFloats( &len, 1 );
            pushInnerChunk( mStream );
            {
                if( (int)ver > (int)SKELETON_VERSION_1_0 )
                {
                    if( anim->getUseBaseKeyFrame() )
                    {
                        size_t size = SSTREAM_OVERHEAD_SIZE;
                        // char* baseAnimationName (including terminator)
                        size += calcStringSize( anim->getBaseKeyFrameAnimationName() );
                        // float baseKeyFrameTime
                        size += sizeof( float );

                        writeChunkHeader( SKELETON_ANIMATION_BASEINFO, size );

                        // char* baseAnimationName (blank for self)
                        writeString( anim->getBaseKeyFrameAnimationName() );

                        // float baseKeyFrameTime
                        float t = (float)anim->getBaseKeyFrameTime();
                        writeFloats( &t, 1 );
                    }
                }

                // Write all tracks
                Animation::OldNodeTrackIterator trackIt = anim->getOldNodeTrackIterator();
                while( trackIt.hasMoreElements() )
                {
                    writeAnimationTrack( pSkel, trackIt.getNext() );
                }
            }
            popInnerChunk( mStream );
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::writeAnimationTrack( const Skeleton *pSkel,
                                                      const OldNodeAnimationTrack *track )
        {
            writeChunkHeader( SKELETON_ANIMATION_TRACK, calcAnimationTrackSize( pSkel, track ) );

            // unsigned short boneIndex     : Index of bone to apply to
            OldBone *bone = (OldBone *)track->getAssociatedNode();
            unsigned short boneid = bone->getHandle();
            writeShorts( &boneid, 1 );
            pushInnerChunk( mStream );
            // Write all keyframes
            for( unsigned short i = 0; i < track->getNumKeyFrames(); ++i )
            {
                writeKeyFrame( pSkel, track->getNodeKeyFrame( i ) );
            }
            popInnerChunk( mStream );
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::writeKeyFrame( const Skeleton *pSkel, const TransformKeyFrame *key )
        {
            writeChunkHeader( SKELETON_ANIMATION_TRACK_KEYFRAME, calcKeyFrameSize( pSkel, key ) );

            // float time                    : The time position (seconds)
            float time = key->getTime();
            writeFloats( &time, 1 );
            // Quaternion rotate            : Rotation to apply at this keyframe
            writeObject( key->getRotation() );
            // Vector3 translate            : Translation to apply at this keyframe
            writeObject( key->getTranslate() );
            // Vector3 scale                : Scale to apply at this keyframe
            if( key->getScale() != Vector3::UNIT_SCALE )
            {
                writeObject( key->getScale() );
            }
        }
        //---------------------------------------------------------------------
        size_t SkeletonSerializer::calcBoneSize( const Skeleton *pSkel, const OldBone *pBone )
        {
            size_t size = calcBoneSizeWithoutScale( pSkel, pBone );

            // scale
            if( pBone->getScale() != Vector3::UNIT_SCALE )
            {
                size += sizeof( float ) * 3;
            }

            return size;
        }
        //---------------------------------------------------------------------
        size_t SkeletonSerializer::calcBoneSizeWithoutScale( const Skeleton *pSkel,
                                                             const OldBone *pBone )
        {
            size_t size = SSTREAM_OVERHEAD_SIZE;

            // TODO: Add this for next skeleton format!
            // Currently it is broken, because to determine that we have scale, it will compare chunk
            // size.
            // size += calcStringSize(pBone->getName());

            // handle
            size += sizeof( unsigned short );

            // position
            size += sizeof( float ) * 3;

            // orientation
            size += sizeof( float ) * 4;

            return size;
        }
        //---------------------------------------------------------------------
        size_t SkeletonSerializer::calcBoneParentSize( const Skeleton *pSkel )
        {
            size_t size = SSTREAM_OVERHEAD_SIZE;

            // handle
            size += sizeof( unsigned short );

            // parent handle
            size += sizeof( unsigned short );

            return size;
        }
        //---------------------------------------------------------------------
        size_t SkeletonSerializer::calcAnimationSize( const Skeleton *pSkel, const Animation *pAnim,
                                                      SkeletonVersion ver )
        {
            size_t size = SSTREAM_OVERHEAD_SIZE;

            // Name, including terminator
            size += calcStringSize( pAnim->getName() );
            // length
            size += sizeof( float );

            if( (int)ver > (int)SKELETON_VERSION_1_0 )
            {
                if( pAnim->getUseBaseKeyFrame() )
                {
                    size += SSTREAM_OVERHEAD_SIZE;
                    // char* baseAnimationName (including terminator)
                    size += calcStringSize( pAnim->getBaseKeyFrameAnimationName() );
                    // float baseKeyFrameTime
                    size += sizeof( float );
                }
            }

            // Nested animation tracks
            Animation::OldNodeTrackIterator trackIt = pAnim->getOldNodeTrackIterator();
            while( trackIt.hasMoreElements() )
            {
                size += calcAnimationTrackSize( pSkel, trackIt.getNext() );
            }

            return size;
        }
        //---------------------------------------------------------------------
        size_t SkeletonSerializer::calcAnimationTrackSize( const Skeleton *pSkel,
                                                           const OldNodeAnimationTrack *pTrack )
        {
            size_t size = SSTREAM_OVERHEAD_SIZE;

            // unsigned short boneIndex     : Index of bone to apply to
            size += sizeof( unsigned short );

            // Nested keyframes
            for( unsigned short i = 0; i < pTrack->getNumKeyFrames(); ++i )
            {
                size += calcKeyFrameSize( pSkel, pTrack->getNodeKeyFrame( i ) );
            }

            return size;
        }
        //---------------------------------------------------------------------
        size_t SkeletonSerializer::calcKeyFrameSize( const Skeleton *pSkel,
                                                     const TransformKeyFrame *pKey )
        {
            size_t size = calcKeyFrameSizeWithoutScale( pSkel, pKey );

            // Vector3 scale                : Scale to apply at this keyframe
            if( pKey->getScale() != Vector3::UNIT_SCALE )
            {
                size += sizeof( float ) * 3;
            }

            return size;
        }
        //---------------------------------------------------------------------
        size_t SkeletonSerializer::calcKeyFrameSizeWithoutScale( const Skeleton *pSkel,
                                                                 const TransformKeyFrame *pKey )
        {
            size_t size = SSTREAM_OVERHEAD_SIZE;

            // float time                    : The time position (seconds)
            size += sizeof( float );
            // Quaternion rotate            : Rotation to apply at this keyframe
            size += sizeof( float ) * 4;
            // Vector3 translate            : Translation to apply at this keyframe
            size += sizeof( float ) * 3;

            return size;
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::readFileHeader( DataStreamPtr &stream )
        {
            unsigned short headerID;

            // Read header ID
            readShorts( stream, &headerID, 1 );

            if( headerID == HEADER_STREAM_ID_EXT )
            {
                // Read version
                String ver = readString( stream );
                if( ( ver != "[Serializer_v1.10]" ) && ( ver != "[Serializer_v1.80]" ) )
                {
                    OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                                 "Invalid file: version incompatible, file reports " + String( ver ),
                                 "Serializer::readFileHeader" );
                }
                mVersion = ver;
            }
            else
            {
                OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Invalid file: no header",
                             "Serializer::readFileHeader" );
            }
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::readBone( DataStreamPtr &stream, Skeleton *pSkel )
        {
            // char* name
            String name = readString( stream );
            // unsigned short handle            : handle of the bone, should be contiguous & start at 0
            unsigned short handle;
            readShorts( stream, &handle, 1 );

            // Create new bone
            OldBone *pBone = pSkel->createBone( name, handle );

            // Vector3 position                 : position of this bone relative to parent
            Vector3 pos;
            readObject( stream, pos );
            pBone->setPosition( pos );
            // Quaternion orientation           : orientation of this bone relative to parent
            Quaternion q;
            readObject( stream, q );
            pBone->setOrientation( q );

#if OGRE_SERIALIZER_VALIDATE_CHUNKSIZE
            // Hack to fix chunk size validation:
            mChunkSizeStack.back() += calcStringSize( name );
#endif
            // TODO: don't depend on mCurrentstreamLen in next skeleton format!
            // Currently we use wrong chunk sizes, but we can't fix it, because we depend on
            // mCurrentstreamLen Do we have scale?
            if( mCurrentstreamLen > calcBoneSizeWithoutScale( pSkel, pBone ) )
            {
                Vector3 scale;
                readObject( stream, scale );
                pBone->setScale( scale );
            }
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::readBoneParent( DataStreamPtr &stream, Skeleton *pSkel )
        {
            // All bones have been created by this point
            OldBone *child, *parent;
            unsigned short childHandle, parentHandle;

            // unsigned short handle             : child bone
            readShorts( stream, &childHandle, 1 );
            // unsigned short parentHandle   : parent bone
            readShorts( stream, &parentHandle, 1 );

            // Find bones
            parent = pSkel->getBone( parentHandle );
            child = pSkel->getBone( childHandle );

            // attach
            parent->addChild( child );
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::readAnimation( DataStreamPtr &stream, Skeleton *pSkel )
        {
            // char* name                       : Name of the animation
            String name;
            name = readString( stream );
            // float length                      : Length of the animation in seconds
            float len;
            readFloats( stream, &len, 1 );

            Animation *pAnim = pSkel->createAnimation( name, len );
            // Read all tracks
            if( !stream->eof() )
            {
                pushInnerChunk( stream );
                unsigned short streamID = readChunk( stream );
                // Optional base info is possible
                if( streamID == SKELETON_ANIMATION_BASEINFO )
                {
                    // char baseAnimationName
                    String baseAnimName = readString( stream );
                    // float baseKeyFrameTime
                    float baseKeyTime;
                    readFloats( stream, &baseKeyTime, 1 );

                    pAnim->setUseBaseKeyFrame( true, baseKeyTime, baseAnimName );

                    if( !stream->eof() )
                    {
                        // Get next stream
                        streamID = readChunk( stream );
                    }
                }

                while( !stream->eof() && streamID == SKELETON_ANIMATION_TRACK )
                {
                    readAnimationTrack( stream, pAnim, pSkel );

                    if( !stream->eof() )
                    {
                        // Get next stream
                        streamID = readChunk( stream );
                    }
                }
                if( !stream->eof() )
                {
                    // Backpedal back to start of this stream if we've found a non-track
                    backpedalChunkHeader( stream );
                }
                popInnerChunk( stream );
            }
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::readAnimationTrack( DataStreamPtr &stream, Animation *anim,
                                                     Skeleton *pSkel )
        {
            // unsigned short boneIndex     : Index of bone to apply to
            unsigned short boneHandle;
            readShorts( stream, &boneHandle, 1 );

            // Find bone
            OldBone *targetBone = pSkel->getBone( boneHandle );

            // Create track
            OldNodeAnimationTrack *pTrack = anim->createOldNodeTrack( boneHandle, targetBone );

            // Keep looking for nested keyframes
            if( !stream->eof() )
            {
                pushInnerChunk( stream );
                unsigned short streamID = readChunk( stream );
                while( !stream->eof() && streamID == SKELETON_ANIMATION_TRACK_KEYFRAME )
                {
                    readKeyFrame( stream, pTrack, pSkel );

                    if( !stream->eof() )
                    {
                        // Get next stream
                        streamID = readChunk( stream );
                    }
                }
                if( !stream->eof() )
                {
                    // Backpedal back to start of this stream if we've found a non-keyframe
                    backpedalChunkHeader( stream );
                }
                popInnerChunk( stream );
            }
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::readKeyFrame( DataStreamPtr &stream, OldNodeAnimationTrack *track,
                                               Skeleton *pSkel )
        {
            // float time                    : The time position (seconds)
            float time;
            readFloats( stream, &time, 1 );

            TransformKeyFrame *kf = track->createNodeKeyFrame( time );

            // Quaternion rotate            : Rotation to apply at this keyframe
            Quaternion rot;
            readObject( stream, rot );
            kf->setRotation( rot );
            // Vector3 translate            : Translation to apply at this keyframe
            Vector3 trans;
            readObject( stream, trans );
            kf->setTranslate( trans );
            // Do we have scale?
            if( mCurrentstreamLen > calcKeyFrameSizeWithoutScale( pSkel, kf ) )
            {
                Vector3 scale;
                readObject( stream, scale );
                kf->setScale( scale );
            }
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::writeSkeletonAnimationLink( const Skeleton *pSkel,
                                                             const LinkedSkeletonAnimationSource &link )
        {
            writeChunkHeader( SKELETON_ANIMATION_LINK, calcSkeletonAnimationLinkSize( pSkel, link ) );

            // char* skeletonName
            writeString( link.skeletonName );
            // float scale
            writeFloats( &( link.scale ), 1 );
        }
        //---------------------------------------------------------------------
        size_t SkeletonSerializer::calcSkeletonAnimationLinkSize(
            const Skeleton *pSkel, const LinkedSkeletonAnimationSource &link )
        {
            size_t size = SSTREAM_OVERHEAD_SIZE;

            // char* skeletonName
            size += link.skeletonName.length() + 1;
            // float scale
            size += sizeof( float );

            return size;
        }
        //---------------------------------------------------------------------
        void SkeletonSerializer::readSkeletonAnimationLink( DataStreamPtr &stream, Skeleton *pSkel )
        {
            // char* skeletonName
            String skelName = readString( stream );
            // float scale
            float scale;
            readFloats( stream, &scale, 1 );

            pSkel->addLinkedSkeletonAnimationSource( skelName, scale );
        }
    }  // namespace v1
}  // namespace Ogre
