
#include "UndoSystem.h"

#include "MainWindow.h"

#include "OgreHlms.h"
#include "OgreHlmsJson.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreRoot.h"

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
void UndoSystem::pushUndoStateMaterialSelect( Ogre::HlmsDatablock *datablock, const bool bRedo,
                                              const bool bClearRedoBuffer )
{
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
        Ogre::HlmsDatablock *datablock = m_mainWindow->getActiveDatablock();
        pushUndoStateMaterialSelect( datablock, &m_redoBuffer == &redoBuffer, false );

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
