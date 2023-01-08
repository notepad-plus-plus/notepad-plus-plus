// this file contains fixes needed for Notepad++ to be built by GCC
// the makefile automatically includes this file

// __try and __except are unknown to GCC, so convert them to something eligible
#define __try try
#define __except(x) catch(...)

#ifndef PROCESSOR_ARCHITECTURE_ARM64
	#define PROCESSOR_ARCHITECTURE_ARM64            12
#endif
