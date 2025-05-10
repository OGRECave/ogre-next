
#pragma once

#include "OgrePrerequisites.h"

struct EditingScope
{
    bool &editing;

    EditingScope( bool &bValue ) : editing( bValue ) { editing = true; }
    ~EditingScope() { editing = false; }
};
