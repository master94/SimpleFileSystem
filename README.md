SimpleFileSystem
================

Simple file system for educational purposes.

================

CMake must be used to build project.
FileSystem structure and functions are described in FS-proj.pdf

FileSystem interface is placed in fs-interface.h

DriverStub emulates work of underlying disk driver. In current edition it just gives access to linear array of bytes.
Current driver stub can be easily replaced with more complicated version.

On the current step of development there is test.c file for tests which are run directly in main.c. Move out file system code to separated dynamic library is a good idea. It is easy to do by adding some lines of code to CMake configs.
