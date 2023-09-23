# Overlays {#v1_Overlays}

v1 Overlays are still supported but they're discouraged since their rendering code is hacked into the v2 pipeline.

Overlays are useful for quickly rendering 2D when needed, but that's it.

They haven't changed from its Ogre counterpart, thus you can visit the [Ogre documentation on Overlays](https://ogrecave.github.io/ogre/api/13/_overlay-_scripts.html).

The main difference is that all of its classes now live in the v1 namespace. e.g. `OverlayElement` is now `v1::OverlayElement`

We recommend [Colibri](https://github.com/darksylinc/colibrigui/) as an alternative which has tight integration into HLMS, is high performance, skins are highly customizable, supports more widgets, and can do advanced text rendering (Unicode, LTR, RTL, CJK, rich text).