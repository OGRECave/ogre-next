
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
        PbsMaterial
    };
}

struct UndoEntry
{
    UndoType::UndoType undoType;

    Ogre::IdString datablockName;
    std::string    json;
    Ogre::String   resourceGroup;

    UndoEntry( UndoType::UndoType _undoType, const Ogre::IdString &_datablockName,
               const std::string &_json, const Ogre::String &_resourceGroup ) :
        undoType( _undoType ),
        datablockName( _datablockName ),
        json( _json ),
        resourceGroup( _resourceGroup )
    {
    }

    bool operator!=( const UndoEntry &other ) const
    {
        return this->undoType != other.undoType || this->datablockName != other.datablockName ||
               this->json != other.json || this->resourceGroup != other.resourceGroup;
    }
};

class UndoSystem
{
    MainWindow *m_mainWindow;

    // UndoEntry              m_currentState;
    std::vector<UndoEntry> m_undoBuffer;
    std::vector<UndoEntry> m_redoBuffer;

    void performUndo( std::vector<UndoEntry> &undoBuffer, std::vector<UndoEntry> &redoBuffer );

public:
    UndoSystem( MainWindow *mainWindow );

    void pushUndoState( Ogre::HlmsDatablock *datablockBase, const bool bRedo = false,
                        const bool bClearRedoBuffer = true );

    void performUndo();
    void performRedo();
};

OGRE_ASSUME_NONNULL_END
