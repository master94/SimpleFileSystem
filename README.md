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

================

DriverStub      - driver sources
FS-proj.pdf     - project description, FS structure, functions
basement.h/c    - low-level functions to work with data blocks, utils, etc
defines.h       - some predefined data, as disk size, block size, descriptors quantity, filename length and etc
fs.c            - core functionality of file system
fs_interface.h  - file system interface
log_wrapper.h/c - wrappers to fs_interface functions for logging purposes
main.c          - entry point, run tests
statuses.h      - return statuses, errors
structs.h       - data structures
tests.h/c       - tests
