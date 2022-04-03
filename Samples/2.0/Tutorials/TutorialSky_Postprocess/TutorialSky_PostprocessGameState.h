
#ifndef _Demo_TutorialSky_PostprocessGameState_H_
#define _Demo_TutorialSky_PostprocessGameState_H_

#include "OgreMesh2.h"
#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    struct CubeVertices
    {
        float px, py, pz;  // Position
        float nx, ny, nz;  // Normals

        CubeVertices() {}
        CubeVertices( float _px, float _py, float _pz, float _nx, float _ny, float _nz ) :
            px( _px ),
            py( _py ),
            pz( _pz ),
            nx( _nx ),
            ny( _ny ),
            nz( _nz )
        {
        }
    };

    class TutorialSky_PostprocessGameState : public TutorialGameState
    {
    public:
        TutorialSky_PostprocessGameState( const Ogre::String &helpDescription );

        void createScene01() override;
    };
}  // namespace Demo

#endif
