
#include "UndoSystem.h"

#include "LightPanel.h"
#include "MainWindow.h"
#include "PbsParametersPanel.h"

#include "OgreEntity.h"
#include "OgreHlms.h"
#include "OgreHlmsJson.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreItem.h"
#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreRoot.h"

#include "rapidjson/document.h"

UndoSystem::UndoSystem( MainWindow *mainWindow ) : m_mainWindow( mainWindow )
{
}
//-----------------------------------------------------------------------------
void UndoSystem::pushUndoState( Ogre::HlmsDatablock *datablockBase, const bool bRedo,
                                const bool bClearRedoBuffer )
{
    if( datablockBase->getCreator()->getType() == Ogre::HLMS_PBS )
    {
        OGRE_ASSERT_HIGH( dynamic_cast<const Ogre::HlmsPbsDatablock *>( datablockBase ) );
        const Ogre::HlmsPbsDatablock *datablock =
            static_cast<const Ogre::HlmsPbsDatablock *>( datablockBase );

        Ogre::HlmsManager *hlmsManager = datablock->getCreator()->getHlmsManager();

        Ogre::String resourceGroup;
        {
            Ogre::Hlms::HlmsDatablockMap::const_iterator itor =
                datablockBase->getCreator()->getDatablockMap().find( datablock->getName() );
            if( itor != datablockBase->getCreator()->getDatablockMap().end() )
                resourceGroup = itor->second.srcResourceGroup;
        }

        Ogre::String jsonString;
        Ogre::HlmsJson hlmsJson( hlmsManager, nullptr );
        hlmsJson.saveMaterial( datablock, jsonString, "" );

        UndoEntry entry( UndoType::PbsMaterial, datablock->getName(), jsonString, resourceGroup );
        if( !bRedo )
        {
            if( m_undoBuffer.empty() || entry != m_undoBuffer.back() )
            {
                m_undoBuffer.push_back( entry );
                if( bClearRedoBuffer )
                    m_redoBuffer.clear();
            }
        }
        else
        {
            if( m_redoBuffer.empty() || entry != m_redoBuffer.back() )
                m_redoBuffer.push_back( entry );
        }
    }
}
//-----------------------------------------------------------------------------
void UndoSystem::pushUndoStateMaterialSelect( const bool bRedo, const bool bClearRedoBuffer )
{
    const Ogre::HlmsDatablock *datablock = m_mainWindow->getActiveDatablock();
    Ogre::IdString datablockName;
    if( datablock )
        datablockName = datablock->getName();

    UndoEntry entry( UndoType::MaterialSelect, datablockName, "", "" );
    if( !bRedo )
    {
        if( m_undoBuffer.empty() || entry != m_undoBuffer.back() )
        {
            m_undoBuffer.push_back( entry );
            if( bClearRedoBuffer )
                m_redoBuffer.clear();
        }
    }
    else
    {
        if( m_redoBuffer.empty() || entry != m_redoBuffer.back() )
            m_redoBuffer.push_back( entry );
    }
}
//-----------------------------------------------------------------------------
void UndoSystem::pushUndoStateMaterialAssign( const Ogre::Item *item, const Ogre::v1::Entity *entity,
                                              const bool bRedo, const bool bClearRedoBuffer )
{
    OGRE_ASSERT_LOW( ( entity && !item ) || ( !entity && item ) );

    Ogre::String meshName;
    Ogre::String resourceGroup;
    if( item )
    {
        meshName = item->getMesh()->getName();
        resourceGroup = item->getMesh()->getGroup();
    }
    else if( entity )
    {
        meshName = entity->getMesh()->getName();
        resourceGroup = entity->getMesh()->getGroup();
    }

    Ogre::MovableObject const *object = item;
    if( !item )
        object = entity;

    std::vector<Ogre::IdString> submeshDatablockNames;
    submeshDatablockNames.reserve( object->mRenderables.size() );
    for( const Ogre::Renderable *renderable : object->mRenderables )
        submeshDatablockNames.push_back( renderable->getDatablock()->getName() );

    UndoEntry entry( UndoType::MaterialAssignment, Ogre::IdString(), meshName, resourceGroup,
                     submeshDatablockNames );
    if( !bRedo )
    {
        if( m_undoBuffer.empty() || entry != m_undoBuffer.back() )
        {
            m_undoBuffer.push_back( entry );
            if( bClearRedoBuffer )
                m_redoBuffer.clear();
        }
    }
    else
    {
        if( m_redoBuffer.empty() || entry != m_redoBuffer.back() )
            m_redoBuffer.push_back( entry );
    }
}
//-----------------------------------------------------------------------------
void UndoSystem::pushUndoMeshSelect( const bool bRedo, const bool bClearRedoBuffer )
{
    const MeshEntry meshEntry = m_mainWindow->getActiveMeshName();
    UndoEntry entry( UndoType::MeshSelect, Ogre::IdString(), meshEntry.name, meshEntry.resourceGroup );
    if( !bRedo )
    {
        if( m_undoBuffer.empty() || entry != m_undoBuffer.back() )
        {
            m_undoBuffer.push_back( entry );
            if( bClearRedoBuffer )
                m_redoBuffer.clear();
        }
    }
    else
    {
        if( m_redoBuffer.empty() || entry != m_redoBuffer.back() )
            m_redoBuffer.push_back( entry );
    }
}
//-----------------------------------------------------------------------------
void UndoSystem::pushUndoLights( const bool bRedo, const bool bClearRedoBuffer )
{
    Ogre::String jsonString;
    jsonString = "{\"u\":0";
    m_mainWindow->getLightPanel()->saveProject( jsonString );
    jsonString += "}";

    const MeshEntry meshEntry = m_mainWindow->getActiveMeshName();
    UndoEntry entry( UndoType::Lights, Ogre::IdString(), jsonString, "" );
    if( !bRedo )
    {
        if( m_undoBuffer.empty() || entry != m_undoBuffer.back() )
        {
            m_undoBuffer.push_back( entry );
            if( bClearRedoBuffer )
                m_redoBuffer.clear();
        }
    }
    else
    {
        if( m_redoBuffer.empty() || entry != m_redoBuffer.back() )
            m_redoBuffer.push_back( entry );
    }
}
//-----------------------------------------------------------------------------
void UndoSystem::performUndo( std::vector<UndoEntry> &undoBuffer, std::vector<UndoEntry> &redoBuffer )
{
    if( undoBuffer.empty() )
        return;

    UndoEntry entry = std::move( undoBuffer.back() );
    undoBuffer.pop_back();

    switch( entry.undoType )
    {
    case UndoType::PbsMaterial:
    {
        Ogre::HlmsManager *hlmsManager = m_mainWindow->getRoot()->getHlmsManager();
        Ogre::HlmsDatablock *currDatablock = hlmsManager->getDatablock( entry.datablockName );
        pushUndoState( currDatablock, &m_redoBuffer == &redoBuffer, false );

        const bool bWasActiveDatablock = m_mainWindow->getActiveDatablock() == currDatablock;

        // We'll be destroying a datablock which may be in use. Prevent crashes.
        Ogre::MovableObject *obj = m_mainWindow->getActiveObject();
        if( obj )
        {
            for( Ogre::Renderable *renderable : obj->mRenderables )
            {
                if( renderable->getDatablock() == currDatablock )
                    renderable->_setNullDatablock();
            }
        }

        Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_PBS );
        hlms->destroyDatablock( entry.datablockName );
        currDatablock = 0;

        Ogre::HlmsJson hlmsJson( hlmsManager, nullptr );
        hlmsJson.loadMaterials( "[FROM MEMORY]", entry.resourceGroup, entry.json.c_str(), "" );
        currDatablock = hlmsManager->getDatablock( entry.datablockName );

        // Restore the (new) datablock back.
        if( obj )
        {
            for( Ogre::Renderable *renderable : obj->mRenderables )
            {
                if( !renderable->getDatablock() )
                    renderable->setDatablock( currDatablock );
            }
        }

        if( bWasActiveDatablock )
            m_mainWindow->setActiveDatablock( currDatablock );
        break;
    }
    case UndoType::MaterialSelect:
    {
        pushUndoStateMaterialSelect( &m_redoBuffer == &redoBuffer, false );

        if( entry.datablockName != Ogre::IdString() )
        {
            Ogre::HlmsManager *hlmsManager = m_mainWindow->getRoot()->getHlmsManager();
            Ogre::HlmsDatablock *prevDatablock =
                hlmsManager->getDatablockNoDefault( entry.datablockName );
            m_mainWindow->setActiveDatablock( prevDatablock );
        }
        else
        {
            m_mainWindow->setActiveDatablock( nullptr );
        }
        break;
    }
    case UndoType::MaterialAssignment:
    {
        const Ogre::Item *item = m_mainWindow->getActiveItem();
        const Ogre::v1::Entity *entity = m_mainWindow->getActiveEntity();
        const MeshEntry meshEntry = m_mainWindow->getActiveMeshName();

        pushUndoStateMaterialAssign( item, entity, &m_redoBuffer == &redoBuffer, false );

        if( meshEntry.name == entry.json && meshEntry.resourceGroup == entry.resourceGroup )
        {
            Ogre::MovableObject *object = m_mainWindow->getActiveObject();
            OGRE_ASSERT( object->mRenderables.size() == entry.submeshDatablockNames.size() );

            Ogre::HlmsManager *hlmsManager = m_mainWindow->getRoot()->getHlmsManager();

            size_t idx = 0u;
            for( Ogre::Renderable *renderable : object->mRenderables )
            {
                Ogre::HlmsDatablock *prevDatablock =
                    hlmsManager->getDatablockNoDefault( entry.submeshDatablockNames[idx++] );
                renderable->setDatablock( prevDatablock );
            }

            m_mainWindow->getPbsParametersPanel()->refreshSubMeshList();
        }
        else
        {
            // Why are we here? Is this even possible? Anyway, destroyed mesh. We can't restore it
            // (assignment is a preview, not permanent. We do not save meshes).
        }
        break;
    }
    case UndoType::MeshSelect:
    {
        pushUndoMeshSelect( &m_redoBuffer == &redoBuffer, false );
        m_mainWindow->setActiveMesh( entry.json, entry.resourceGroup );
        break;
    }
    case UndoType::Lights:
    {
        pushUndoMeshSelect( &m_redoBuffer == &redoBuffer, false );
        rapidjson::Document d;
        d.Parse( entry.json.c_str() );
        if( !d.HasParseError() )
            m_mainWindow->getLightPanel()->loadProject( d );
        break;
    }
    }
}
//-----------------------------------------------------------------------------
void UndoSystem::performUndo()
{
    performUndo( m_undoBuffer, m_redoBuffer );
}
//-----------------------------------------------------------------------------
void UndoSystem::performRedo()
{
    performUndo( m_redoBuffer, m_undoBuffer );
}
//-----------------------------------------------------------------------------
void UndoSystem::clearBuffers()
{
    m_undoBuffer.clear();
    m_redoBuffer.clear();
}
