$Id$

Notes on Compiling for Win32
============================

On some versions of Windows, libc streams cannot be passed
between shared libraries, and since libpng uses libc streams,
textures don't work.
