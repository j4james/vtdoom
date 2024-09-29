VT DOOM
=======

![Screenshot](screenshot.png)

This is a port of the [DOOM] video game for VT terminals supporting the Sixel
graphics protocol. It's mostly playable with standard VT input, but ideally
requires a terminal supporting the win32 input mode used by Windows Terminal.

It was developed with the help of the excellent [PureDOOM] single header port.

[DOOM]: https://en.wikipedia.org/wiki/Doom_(1993_video_game)
[PureDOOM]: https://github.com/Daivuk/PureDOOM


Download
--------

The latest binaries can be found on GitHub at the following url:

https://github.com/j4james/vtdoom/releases/latest

For Linux download `vtdoom`, and for Windows download `vtdoom.exe`.

You'll also need a copy of the `doom1.wad` data file from here:

https://doomwiki.org/wiki/DOOM1.WAD


Build Instructions
------------------

If you want to build this yourself, you'll need [CMake] version 3.15 or later
and a C++ compiler supporting C++20 or later.

1. Download or clone the source:  
   `git clone https://github.com/j4james/vtdoom.git`

2. Change into the build directory:  
   `cd vtdoom/build`

3. Generate the build project:  
   `cmake -D CMAKE_BUILD_TYPE=Release ..`

4. Start the build:  
   `cmake --build . --config Release`

[CMake]: https://cmake.org/


See Also
--------

This project was inspired by [XtermDOOM].

[XtermDOOM]: https://gitlab.com/AutumnMeowMeow/xtermdoom


License
-------

The VT DOOM source code and binaries are released under the GPL-2.0 License.
See the [LICENSE] file for full license details.

[LICENSE]: LICENSE.txt
