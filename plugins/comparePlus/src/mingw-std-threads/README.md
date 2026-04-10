mingw-std-threads
=================

Implementation of standard C++11 threading classes, which are currently still missing on MinGW GCC.

Target Windows version
----------------------
This implementation should work with Windows XP (regardless of service pack), or newer.
The library automatically detects the version of Windows that is being targeted (at compile time), and selects an implementation that takes advantage of available Windows features.
In MinGW GCC, the target Windows version may optionally be selected by the command-line option `-D _WIN32_WINNT=...`.
Use `0x0600` for Windows Vista, or `0x0601` for Windows 7.
See "[Modifying `WINVER` and `_WIN32_WINNT`](https://docs.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt)" for more details.

Usage
-----

This is a header-only library. To use, just include the corresponding `mingw.xxx.h file`, where `xxx` would be the name of the standard header that you would normally include.

For example, `#include "mingw.thread.h"` replaces `#include <thread>`.

Compatibility
-------------

This code has been tested to work with MinGW-w64 5.3.0, but should work with any other MinGW version that has the `std` threading classes missing, has C++11 support for lambda functions, variadic templates, and has working mutex helper classes in `<mutex>`.

Switching from the win32-pthread based implementation
-----------------------------------------------------
It seems that recent versions of MinGW-w64 include a Win32 port of pthreads, and have the `std::thread`, `std::mutex`, etc. classes implemented and working based on that compatibility
layer.
That is a somewhat heavier implementation, as it relies on an abstraction layer, so you may still want to use this implementation for efficiency purposes.
Unfortunately you can't use this library standalone and independent of the system `<mutex>` headers, as it relies on those headers for `std::unique_lock` and other non-trivial utility classes.
In that case you will need to edit the `c++-config.h` file of your MinGW setup and comment out the definition of _GLIBCXX_HAS_GTHREADS.
This will cause the system headers not to define the actual `thread`, `mutex`, etc. classes, but still define the necessary utility classes.

Why MinGW has no threading classes 
----------------------------------
It seems that for cross-platform threading implementation, the GCC standard library relies on the gthreads/pthreads library.
If this library is not available, as is the case with MinGW, the classes `std::thread`, `std::mutex`, `std::condition_variable` are not defined.
However, various usable helper classes are still defined in the system headers.
Hence, this implementation does not re-define them, and instead includes those headers.

