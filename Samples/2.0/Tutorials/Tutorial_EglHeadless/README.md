# EGL Headless sample

This sample shows how to run Ogre in EGL headless, which can be useful
for running in a VM or in the Cloud.

Although it could work in Windows, it has been mostly geared towards Linux.

X11 does not need to be running, however Mesa needs to be installed and
Mesa often depends on X11 libraries which means they must be installed.
Additionally Ogre needs `glXGetProcAddressARB` in
`RenderSystems/GL3Plus/src/gl3w.cpp` although this could be changed for
eglGetProcAddress if complete X11-independence is needed.

This sample has been confirmed to be working over SSH on a headless VM with
no HW support; as well as tested over an SSH session on a real machine
using the DRM interface using a real GPU.

`ldd` can be a great tool for determining required missing libraries:

```
ldd Sample_Tutorial_EglHeadless
	linux-vdso.so.1 (0x00007ffecc1d8000)
	libOgreHlmsPbs_d.so.2.2.5 => /home/matias/OgreSrc/build/Debug/lib/libOgreHlmsPbs_d.so.2.2.5 (0x00007f8a784eb000)
	libOgreHlmsUnlit_d.so.2.2.5 => /home/matias/OgreSrc/build/Debug/lib/libOgreHlmsUnlit_d.so.2.2.5 (0x00007f8a782ae000)
	libOgreMain_d.so.2.2.5 => /home/matias/OgreSrc/build/Debug/lib/libOgreMain_d.so.2.2.5 (0x00007f8a77659000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f8a772d0000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f8a770b8000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f8a76cc7000)
	libOgrePlanarReflections_d.so.2.2.5 => /home/matias/OgreSrc/build/Debug/lib/libOgrePlanarReflections_d.so.2.2.5 (0x00007f8a76aae000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f8a76710000)
	libX11.so.6 => /usr/lib/x86_64-linux-gnu/libX11.so.6 (0x00007f8a763d8000)
	libXt.so.6 => /usr/lib/x86_64-linux-gnu/libXt.so.6 (0x00007f8a7616f000)
	libXaw.so.7 => /usr/lib/x86_64-linux-gnu/libXaw.so.7 (0x00007f8a75efb000)
	libXrandr.so.2 => /usr/lib/x86_64-linux-gnu/libXrandr.so.2 (0x00007f8a75cf0000)
	libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007f8a75ad1000)
	libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007f8a758cd000)
	libfreeimage.so.3 => /usr/lib/x86_64-linux-gnu/libfreeimage.so.3 (0x00007f8a7561e000)
	libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1 (0x00007f8a75401000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f8a789f1000)
	libxcb.so.1 => /usr/lib/x86_64-linux-gnu/libxcb.so.1 (0x00007f8a751d9000)
	libSM.so.6 => /usr/lib/x86_64-linux-gnu/libSM.so.6 (0x00007f8a74fd1000)
	libICE.so.6 => /usr/lib/x86_64-linux-gnu/libICE.so.6 (0x00007f8a74db6000)
	libXext.so.6 => /usr/lib/x86_64-linux-gnu/libXext.so.6 (0x00007f8a74ba4000)
	libXmu.so.6 => /usr/lib/x86_64-linux-gnu/libXmu.so.6 (0x00007f8a7498b000)
	libXpm.so.4 => /usr/lib/x86_64-linux-gnu/libXpm.so.4 (0x00007f8a74779000)
	libXrender.so.1 => /usr/lib/x86_64-linux-gnu/libXrender.so.1 (0x00007f8a7456f000)
	libjxrglue.so.0 => /usr/lib/x86_64-linux-gnu/libjxrglue.so.0 (0x00007f8a7434f000)
	libjpeg.so.8 => /usr/lib/x86_64-linux-gnu/libjpeg.so.8 (0x00007f8a740e7000)
	libopenjp2.so.7 => /usr/lib/x86_64-linux-gnu/libopenjp2.so.7 (0x00007f8a73e91000)
	libpng16.so.16 => /usr/lib/x86_64-linux-gnu/libpng16.so.16 (0x00007f8a73c5f000)
	libraw.so.16 => /usr/lib/x86_64-linux-gnu/libraw.so.16 (0x00007f8a7398c000)
	libtiff.so.5 => /usr/lib/x86_64-linux-gnu/libtiff.so.5 (0x00007f8a73715000)
	libwebpmux.so.3 => /usr/lib/x86_64-linux-gnu/libwebpmux.so.3 (0x00007f8a7350b000)
	libwebp.so.6 => /usr/lib/x86_64-linux-gnu/libwebp.so.6 (0x00007f8a732a2000)
	libIlmImf-2_2.so.22 => /usr/lib/x86_64-linux-gnu/libIlmImf-2_2.so.22 (0x00007f8a72dde000)
	libHalf.so.12 => /usr/lib/x86_64-linux-gnu/libHalf.so.12 (0x00007f8a72b9b000)
	libIex-2_2.so.12 => /usr/lib/x86_64-linux-gnu/libIex-2_2.so.12 (0x00007f8a7297d000)
	libXau.so.6 => /usr/lib/x86_64-linux-gnu/libXau.so.6 (0x00007f8a72779000)
	libXdmcp.so.6 => /usr/lib/x86_64-linux-gnu/libXdmcp.so.6 (0x00007f8a72573000)
	libuuid.so.1 => /lib/x86_64-linux-gnu/libuuid.so.1 (0x00007f8a7236c000)
	libbsd.so.0 => /lib/x86_64-linux-gnu/libbsd.so.0 (0x00007f8a72157000)
	libjpegxr.so.0 => /usr/lib/x86_64-linux-gnu/libjpegxr.so.0 (0x00007f8a71f23000)
	liblcms2.so.2 => /usr/lib/x86_64-linux-gnu/liblcms2.so.2 (0x00007f8a71ccb000)
	libgomp.so.1 => /usr/lib/x86_64-linux-gnu/libgomp.so.1 (0x00007f8a71a9c000)
	liblzma.so.5 => /lib/x86_64-linux-gnu/liblzma.so.5 (0x00007f8a71876000)
	libjbig.so.0 => /usr/lib/x86_64-linux-gnu/libjbig.so.0 (0x00007f8a71668000)
	libIlmThread-2_2.so.12 => /usr/lib/x86_64-linux-gnu/libIlmThread-2_2.so.12 (0x00007f8a71461000)
	librt.so.1 => /lib/x86_64-linux-gnu/librt.so.1 (0x00007f8a71259000)
```

This tutorial is based on Tutorial00_Basic, tweaked to select between EGL devices and
avoid relying on the default configurator (which requires an active X11 session to run).