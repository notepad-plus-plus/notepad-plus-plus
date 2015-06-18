// YUNI's default license is the GNU Lesser Public License (LGPL), with some
// exclusions (see below). This basically means that you can get the full source
// code for nothing, so long as you adhere to a few rules.
// 
// Under the LGPL you may use YUNI for any purpose you wish, and modify it if you
// require, as long as you:
// 
// Pass on the (modified) YUNI source code with your software, with original 
// copyrights intact :
//  * If you distribute electronically, the source can be a separate download
//    (either from your own site if you modified YUNI, or to the official YUNI
//    website if you used an unmodified version) – just include a link in your
//    documentation
//  * If you distribute physical media, the YUNI source that you used to build
//    your application should be included on that media
// Make it clear where you have customised it.
// 
// In addition to the LGPL license text, the following exceptions / clarifications
// to the LGPL conditions apply to YUNI:
// 
//  * Making modifications to YUNI configuration files, build scripts and
//    configuration headers such as yuni/platform.h in order to create a
//    customised build setup of YUNI with the otherwise unmodified source code,
//    does not constitute a derived work
//  * Building against YUNI headers which have inlined code does not constitute a
//    derived work
//  * Code which subclasses YUNI classes outside of the YUNI libraries does not
//    form a derived work
//  * Statically linking the YUNI libraries into a user application does not make
//    the user application a derived work.
//  * Using source code obsfucation on the YUNI source code when distributing it
//    is not permitted.
// As per the terms of the LGPL, a "derived work" is one for which you have to
// distribute source code for, so when the clauses above define something as not
// a derived work, it means you don't have to distribute source code for it.
// However, the original YUNI source code with all modifications must always be
// made available.

#include "mutex.h"
#include <cassert>
#include <iostream>
#include <cerrno>

#if YUNI_ATOMIC_MUST_USE_MUTEX != 0
#warning Atomic types must ue mutex. the implementation should be checked YUNI_OS_GCC_VERSION
#endif



namespace Yuni
{

	enum // anonymous
	{
		/*!
		** \brief The spin count for the critical section object
		**
		** On single-processor systems, the spin count is ignored and the critical section
		** spin count is set to 0 (zero). On multiprocessor systems, if the critical section
		** is unavailable, the calling thread spinsdwSpinCount times before performing a
		** wait operation on a semaphore associated with the critical section. If the critical
		** section becomes free during the spin operation, the calling thread avoids the
		** wait operation.
		** \see http://msdn.microsoft.com/en-us/library/ms683476%28v=vs.85%29.aspx
		*/
		spinCount = 3000,
	};



	inline void Mutex::destroy()
	{
		# ifndef YUNI_NO_THREAD_SAFE
		# ifdef YUNI_OS_WINDOWS
		DeleteCriticalSection(&pSection);
		# else
		switch (::pthread_mutex_destroy(&pLock))
		{
			case 0: // Ok good
			{
				break;
			}
			// If an error happens, we will let the program continue but
			// it can becaome ugly around here...
			case EBUSY:
			{
				std::cerr << "\nattempt to destroy a mutex while it is locked or referenced\n";
				assert(false and "attempt to destroy a mutex while it is locked or referenced");
				break;
			}
			default:
			{
				std::cerr << "\nfailed to destroy a mutex\n";
				assert(false and "\nfailed to destroy a mutex\n");
				break;
			}
		}
		::pthread_mutexattr_destroy(&pAttr);
		# endif
		# endif // no thread safe
	}


	inline void Mutex::copy(const Mutex& rhs)
	{
		# ifndef YUNI_NO_THREAD_SAFE
		# ifdef YUNI_OS_WINDOWS
		InitializeCriticalSectionAndSpinCount(&pSection, spinCount);
		(void) rhs; // unused
		# else
		::pthread_mutexattr_init(&pAttr);
		int type; // = PTHREAD_MUTEX_NORMAL;
		if (0 == ::pthread_mutexattr_gettype(&rhs.pAttr, &type))
		{
			if (PTHREAD_MUTEX_RECURSIVE == type)
			{
				# if defined(YUNI_OS_DARWIN) or defined(YUNI_OS_FREEBSD) or defined(YUNI_OS_SOLARIS) or defined(YUNI_OS_SUNOS) or defined(YUNI_OS_HAIKU) or defined(YUNI_OS_CYGWIN)
				::pthread_mutexattr_settype(&pAttr, PTHREAD_MUTEX_RECURSIVE);
				# else
				::pthread_mutexattr_settype(&pAttr, PTHREAD_MUTEX_RECURSIVE_NP);
				# endif
			}
		}
		::pthread_mutex_init(& pLock, &pAttr);
		# endif
		# else
		(void) rhs; // unused
		# endif // no thread safe
	}



	Mutex::Mutex(const Mutex& rhs)
	{
		copy(rhs);
	}


	Mutex::~Mutex()
	{
		destroy();
	}


	Mutex::Mutex(bool recursive)
	{
		# ifndef YUNI_NO_THREAD_SAFE
		# ifdef YUNI_OS_WINDOWS
		(void) recursive; // already recursive on Windows
		InitializeCriticalSectionAndSpinCount(&pSection, spinCount);
		# else
		::pthread_mutexattr_init(&pAttr);
		if (recursive)
		{
			# if defined(YUNI_OS_DARWIN) or defined(YUNI_OS_FREEBSD) or defined(YUNI_OS_SOLARIS) or defined(YUNI_OS_SUNOS) or defined(YUNI_OS_HAIKU) or defined(YUNI_OS_CYGWIN)
			::pthread_mutexattr_settype(&pAttr, PTHREAD_MUTEX_RECURSIVE);
			# else
			::pthread_mutexattr_settype(&pAttr, PTHREAD_MUTEX_RECURSIVE_NP);
			# endif
		}
		::pthread_mutex_init(&pLock, &pAttr);
		# endif
		# else
		(void) recursive;
		# endif
	}


	Mutex& Mutex::operator = (const Mutex& rhs)
	{
		// We will recreate the mutex
		destroy();
		copy(rhs);
		return *this;
	}





} // namespace Yuni
