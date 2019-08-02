
#ifndef _Demo_UnalignedBufferUavCopyGameState_H_
#define _Demo_UnalignedBufferUavCopyGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
	class UnalignedBufferUavCopyGameState : public TutorialGameState
	{
		void verifyBuffer( const Ogre::uint16 *referenceBuffer, Ogre::IndexBufferPacked *indexBuffer,
						   Ogre::UavBufferPacked *uavBuffer, size_t dstElemStart,
						   size_t srcElemStart, size_t srcNumElements );
    public:
		UnalignedBufferUavCopyGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);
		virtual void destroyScene(void);
    };
}

#endif
