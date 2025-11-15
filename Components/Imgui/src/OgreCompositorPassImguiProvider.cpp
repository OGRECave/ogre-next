/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2025 Torus Knot Software Ltd

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

Original implementation by Crashy.
Further enhancements by edherbert.
General fixes: Various people https://forums.ogre3d.org/viewtopic.php?t=93889
Updated for Ogre-Next 2.3 by Vian https://forums.ogre3d.org/viewtopic.php?t=96798
Reworked for Ogre-Next 4.0 by Matias N. Goldberg.

Based on the implementation in https://github.com/edherbert/ogre-next-imgui
-----------------------------------------------------------------------------
*/

#include "OgreCompositorPassImguiProvider.h"

#include "OgreCompositorPassImgui.h"
#include "OgreCompositorPassImguiDef.h"
#include "OgreScriptTranslator.h"

namespace Ogre
{
    CompositorPassImguiProvider::CompositorPassImguiProvider( ImguiManager *imguiManager ) :
        mImguiManager( imguiManager )
    {
    }
    //-------------------------------------------------------------------------
    CompositorPassDef *CompositorPassImguiProvider::addPassDef( CompositorPassType passType,
                                                                IdString customId,
                                                                CompositorTargetDef *parentTargetDef,
                                                                CompositorNodeDef *parentNodeDef )
    {
        if( customId == "dear_imgui" )
        {
            return OGRE_NEW CompositorPassImguiDef( parentTargetDef );
        }

        return 0;
    }
    //-------------------------------------------------------------------------
    CompositorPass *CompositorPassImguiProvider::addPass( const CompositorPassDef *definition,
                                                          Camera *defaultCamera,
                                                          CompositorNode *parentNode,
                                                          const RenderTargetViewDef *rtvDef,
                                                          SceneManager *sceneManager )
    {
        OGRE_ASSERT_HIGH( dynamic_cast<const CompositorPassImguiDef *>( definition ) );
        const CompositorPassImguiDef *imguiDef =
            static_cast<const CompositorPassImguiDef *>( definition );
        return OGRE_NEW CompositorPassImgui( imguiDef, defaultCamera, sceneManager, rtvDef, parentNode,
                                             mImguiManager );
    }
    //-------------------------------------------------------------------------
    static bool ScriptTranslatorGetBoolean( const AbstractNodePtr &node, bool *result )
    {
        if( node->type != ANT_ATOM )
            return false;
        const AtomAbstractNode *atom = (const AtomAbstractNode *)node.get();
        if( atom->id == 1 || atom->id == 2 )
        {
            *result = atom->id == 1 ? true : false;
            return true;
        }
        return false;
    }
    //-------------------------------------------------------------------------
    void CompositorPassImguiProvider::translateCustomPass( ScriptCompiler *compiler,
                                                           const AbstractNodePtr &node,
                                                           IdString customId,
                                                           CompositorPassDef *customPassDef )
    {
        if( customId != "dear_imgui" )
            return;  // Custom pass not created by us

        CompositorPassImguiDef *imguiDef = static_cast<CompositorPassImguiDef *>( customPassDef );

        ObjectAbstractNode *obj = reinterpret_cast<ObjectAbstractNode *>( node.get() );

        obj->context = Any( static_cast<CompositorPassDef *>( imguiDef ) );

        AbstractNodeList::const_iterator itor = obj->children.begin();
        AbstractNodeList::const_iterator endt = obj->children.end();

        while( itor != endt )
        {
            if( ( *itor )->type == ANT_OBJECT )
            {
                ObjectAbstractNode *childObj = reinterpret_cast<ObjectAbstractNode *>( itor->get() );

                if( childObj->id == ID_LOAD )
                {
                    CompositorLoadActionTranslator compositorLoadActionTranslator;
                    compositorLoadActionTranslator.translate( compiler, *itor );
                }
                else if( childObj->id == ID_STORE )
                {
                    CompositorStoreActionTranslator compositorStoreActionTranslator;
                    compositorStoreActionTranslator.translate( compiler, *itor );
                }
            }
            else if( ( *itor )->type == ANT_PROPERTY )
            {
                const PropertyAbstractNode *prop =
                    reinterpret_cast<const PropertyAbstractNode *>( itor->get() );
                if( prop->name == "sets_resolution" )
                {
                    if( prop->values.size() != 1u ||
                        !ScriptTranslatorGetBoolean( prop->values.front(), &imguiDef->mSetsResolution ) )
                    {
                        compiler->addError( ScriptCompiler::CE_STRINGEXPECTED, obj->file, obj->line,
                                            "sets_resolution expects a boolean value" );
                    }
                }
            }
            ++itor;
        }
    }
}  // namespace Ogre
