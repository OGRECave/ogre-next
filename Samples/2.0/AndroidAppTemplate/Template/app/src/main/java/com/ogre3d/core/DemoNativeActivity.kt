/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-present Torus Knot Software Ltd

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

package com.ogre3d.core

import android.app.AlertDialog
import android.app.NativeActivity
import android.graphics.Rect

class DemoNativeActivity : NativeActivity() {
    fun showNoVulkanAlert(errorMsg: String, detailedErrorMsg: String) {
        runOnUiThread {
            var dlg = AlertDialog.Builder(this)
                .setMessage("This game requires Vulkan device support. We are sorry.\nThis app cannot be run on this device. Rendering Engine said:\n\n" + errorMsg)
                .setCancelable(false)
                .create();
            dlg.show();

            // The default dialog is waaay too small. Make it bigger.
            val displayRectangle = Rect()
            window.getDecorView().getWindowVisibleDisplayFrame(displayRectangle);
            dlg.window?.setLayout(
                displayRectangle.width() * 80 / 100,
                displayRectangle.height() * 80 / 100
            );
        }
    }

    fun getCacheFolder(): String {
        return applicationContext.cacheDir.canonicalPath
    }
}
