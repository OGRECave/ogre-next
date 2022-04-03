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
#include "OgreErrorDialog.h"

#include <gtk/gtk.h>
#include <iostream>

namespace Ogre
{
    extern bool __gtk_init_once();

    //---------------------------------------------------------------------------//
    ErrorDialog::ErrorDialog() {}

    //---------------------------------------------------------------------------//
    void ErrorDialog::display( const String &errorMessage, String logName )
    {
        if( !__gtk_init_once() )
        {
            std::cerr << "*** ERROR: " << errorMessage << std::endl;
            return;
        }

        GtkWidget *dialog = gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_OK, errorMessage.c_str() );

        gtk_dialog_run( GTK_DIALOG( dialog ) );
        gtk_widget_destroy( dialog );

        // Wait for all gtk events to be consumed ...
        while( gtk_events_pending() )
            gtk_main_iteration_do( FALSE );
    }

}  // namespace Ogre
