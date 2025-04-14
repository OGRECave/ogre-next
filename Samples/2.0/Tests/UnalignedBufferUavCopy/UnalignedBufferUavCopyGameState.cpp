
#include "UnalignedBufferUavCopyGameState.h"
#include "GraphicsSystem.h"

#include "Vao/OgreAsyncTicket.h"
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreUavBufferPacked.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreVaoManager.h"

#include "OgreLwString.h"

#include "OgreRoot.h"

using namespace Demo;

namespace Demo
{
    UnalignedBufferUavCopyGameState::UnalignedBufferUavCopyGameState(
        const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription )
    {
    }
    //-----------------------------------------------------------------------------------
    void UnalignedBufferUavCopyGameState::verifyBuffer( const Ogre::uint16 *referenceBuffer,
                                                        Ogre::IndexBufferPacked *indexBuffer,
                                                        Ogre::UavBufferPacked *uavBuffer,
                                                        size_t dstElemStart, size_t srcElemStart,
                                                        size_t srcNumElements )
    {
        indexBuffer->copyTo( uavBuffer, dstElemStart, srcElemStart, srcNumElements );

        Ogre::AsyncTicketPtr readTicket = uavBuffer->readRequest( 0u, uavBuffer->getNumElements() );

        const Ogre::uint16 *dataBuffer = reinterpret_cast<const Ogre::uint16 *>( readTicket->map() );

        for( size_t i = 0u; i < srcNumElements; ++i )
        {
            const size_t refIdx = srcElemStart + i;
            const size_t uavIdx = ( dstElemStart << 1u ) + i;
            if( referenceBuffer[refIdx] != dataBuffer[uavIdx] )
            {
                char tmpBuffer[512];
                Ogre::LwString txt( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
                txt.a( "Failed referenceBuffer[i] != dataBuffer[i]. Expecting ", referenceBuffer[refIdx],
                       " got ", dataBuffer[uavIdx] );
                OGRE_EXCEPT( Ogre::Exception::ERR_INVALID_STATE, txt.c_str(),
                             "UnalignedBufferUavCopyGameState::verifyBuffer" );
            }
        }

        readTicket->unmap();
    }
    //-----------------------------------------------------------------------------------
    void UnalignedBufferUavCopyGameState::createScene01()
    {
        Ogre::RenderSystem *renderSystem = mGraphicsSystem->getRoot()->getRenderSystem();

        const size_t c_refBufferCount = 10001;
        Ogre::uint16 *referenceBuffer[2];
        referenceBuffer[0] = new Ogre::uint16[c_refBufferCount];
        referenceBuffer[1] = new Ogre::uint16[c_refBufferCount];

        for( size_t i = 0u; i < c_refBufferCount; ++i )
        {
            referenceBuffer[0][i] = static_cast<Ogre::uint16>( c_refBufferCount + i );
            referenceBuffer[1][i] = static_cast<Ogre::uint16>( 50 + i );
        }

        Ogre::VaoManager *vaoManager = renderSystem->getVaoManager();

        Ogre::IndexBufferPacked *indexBuffer[2];
        for( size_t i = 0u; i < 2u; ++i )
        {
            indexBuffer[i] =
                vaoManager->createIndexBuffer( Ogre::IndexBufferPacked::IT_16BIT, c_refBufferCount,
                                               Ogre::BT_DEFAULT, referenceBuffer[i], false );
        }
        Ogre::UavBufferPacked *uavBuffer =
            vaoManager->createUavBuffer( ( c_refBufferCount + 1u ) / 2u, 4u, 0, 0, false );

        size_t idx = 0;
        // Not all platforms initialize buffers to 0, so swap the buffers and test both in case
        // the buffer originally had coincidentally the same content, and the copy isn't
        // actually working.
        verifyBuffer( referenceBuffer[idx], indexBuffer[idx], uavBuffer, 0u, 0u, c_refBufferCount );
        idx = !idx;
        verifyBuffer( referenceBuffer[idx], indexBuffer[idx], uavBuffer, 0u, 0u, c_refBufferCount );
        idx = !idx;

        verifyBuffer( referenceBuffer[idx], indexBuffer[idx], uavBuffer, 0u, 1u, 7u );
        idx = !idx;

        verifyBuffer( referenceBuffer[idx], indexBuffer[idx], uavBuffer, 5u, 1u, 7u );
        idx = !idx;
        verifyBuffer( referenceBuffer[idx], indexBuffer[idx], uavBuffer, 5u, 1u, 7u );
        idx = !idx;

        vaoManager->destroyUavBuffer( uavBuffer );
        for( size_t i = 0u; i < 2u; ++i )
        {
            vaoManager->destroyIndexBuffer( indexBuffer[i] );
            delete[] referenceBuffer[i];
        }

        TutorialGameState::createScene01();

        mGraphicsSystem->setQuit();
    }
    //-----------------------------------------------------------------------------------
    void UnalignedBufferUavCopyGameState::destroyScene() {}
}  // namespace Demo
