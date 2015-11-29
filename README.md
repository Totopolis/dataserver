Dataserver is open source data warehouse around sql server mdf structure
written in C++11/C++14.

SUPPORTED PLATFORMS
===================

Platforms:

*   **Linux** [![Build Status](https://travis-ci.org/Totopolis/dataserver.svg?branch=master)](https://travis-ci.org/Totopolis/dataserver)
*   **Windows** [![Build status](https://ci.appveyor.com/api/projects/status/r0xsqmreyx14xmbp?svg=true)](https://ci.appveyor.com/project/Totopolis/dataserver)
*   **OS X** [![Build Status](https://travis-ci.org/Totopolis/dataserver.svg?branch=master)](https://travis-ci.org/Totopolis/dataserver)
*   **FreeBSD**

INSTALLATION
============

Minimal dependencies
--------------------

-   C++ compiler with good C++11 support. Compilers which are tested to have
    everything needed are **Clang** >= 3.7 and **MSVC** 2015.
*   **CMake** >= 2.6

Compilation
-----------

The application can be built using these four commands:

    mkdir build
    cd build
    cmake .. 
    make

LICENSE
=======

Dataserver is licensed under MIT license, see [LICENSE](LICENSE) file for
details.