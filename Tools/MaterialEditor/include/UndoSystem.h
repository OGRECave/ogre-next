
#pragma once

#include "MaterialEditorCommon.h"

#include "OgreHlmsPbsPrerequisites.h"
#include "OgreIdString.h"

OGRE_ASSUME_NONNULL_BEGIN

class MainWindow;

namespace UndoType
{
    enum UndoType
    {
        PbsMaterial,
        MaterialSelect,
        MaterialAssignment,
        MeshSelect,
    };
}

struct UndoEntry
{
    UndoType::UndoType undoType;

    Ogre::IdString datablockName;
    std::string    json;
    Ogre::String   resourceGroup;

    std::vector<Ogre::IdString> submeshDatablockNames;

    UndoEntry(
        UndoType::UndoType _undoType, const Ogre::IdString &_datablockName, const std::string &_json,
        const Ogre::String                &_resourceGroup,
        const std::vector<Ogre::IdString> &_submeshDatablockNames = std::vector<Ogre::IdString>() ) :
        undoType( _undoType ),
        datablockName( _datablockName ),
        json( _json ),
        resourceGroup( _resourceGroup ),
        submeshDatablockNames( _submeshDatablockNames )
    {
    }

    bool operator!=( const UndoEntry &other ) const
    {
        return this->undoType != other.undoType || this->datablockName != other.datablockName ||
               this->json != other.json || this->resourceGroup != other.resourceGroup ||
               this->submeshDatablockNames != other.submeshDatablockNames;
    }
};

class UndoSystem
{
    MainWindow *m_mainWindow;

    std::vector<UndoEntry> m_undoBuffer;
    std::vector<UndoEntry> m_redoBuffer;

    void performUndo( std::vector<UndoEntry> &undoBuffer, std::vector<UndoEntry> &redoBuffer );

public:
    UndoSystem( MainWindow *mainWindow );

    void pushUndoState( Ogre::HlmsDatablock *datablockBase, const bool bRedo = false,
                        const bool bClearRedoBuffer = true );

    void pushUndoStateMaterialSelect( const bool bRedo = false, const bool bClearRedoBuffer = true );

    void pushUndoStateMaterialAssign( const Ogre::Item *ogre_nullable       item,
                                      const Ogre::v1::Entity *ogre_nullable entity,
                                      const bool bRedo = false, const bool bClearRedoBuffer = true );

    void pushUndoMeshSelect( const bool bRedo = false, const bool bClearRedoBuffer = true );

    void performUndo();
    void performRedo();

    void clearBuffers();
};

OGRE_ASSUME_NONNULL_END
