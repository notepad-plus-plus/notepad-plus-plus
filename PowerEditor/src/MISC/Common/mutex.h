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

#pragma once
#define YUNI_OS_WINDOWS
#define YUNI_HAS_CPP_MOVE
#define YUNI_DECL

#include <windows.h>



namespace Yuni
{

	/*!
	** \brief  Mechanism to avoid the simultaneous use of a common resource
	**
	** \ingroup Threads
	*/
	class YUNI_DECL Mutex final
	{
	public:
		/*!
		** \brief A class-level locking mechanism
		**
		** A class-level locking operation locks all objects in a given class during that operation
		*/
		template<class T>
		class ClassLevelLockable
		{
		public:
			//! A dedicated mutex for the class T
			static Mutex mutex;

		}; // class ClassLevelLockable


	public:
		//! \name Constructor & Destructor
		//@{
		/*!
		** \brief Default constructor
		**
		** Recursive by default to keep homogeneous behavior between
		** platforms.
		*/
		explicit Mutex(bool recursive = true);
		/*!
		** \brief Copy constructor
		**
		** This constructor does actually nothing but it allows the compilation
		** of other classes which would implement a copy constructor
		*/
		Mutex(const Mutex&);

		# ifdef YUNI_HAS_CPP_MOVE
		// an OS's native mutex must have invariant address and thus can not be moved
		Mutex(Mutex&&) = delete;
		#endif

		/*!
		** \brief Destructor
		*/
		~Mutex();
		//@}

		//! \name Lock & Unlock
		//@{
		/*!
		** \brief Lock the mutex
		*/
		void lock();

		/*!
		** \brief Try to lock the mutex
		**
		** \return True if the mutex has been locked, false otherwise
		*/
		bool trylock();

		/*!
		** \brief Release the lock
		*/
		void unlock();
		//@}

		# ifndef YUNI_NO_THREAD_SAFE
		# ifndef YUNI_OS_WINDOWS
		//! \name Native
		//@{
		//! Get the original PThread mutex
		::pthread_mutex_t& pthreadMutex();
		//! Get the original PThread mutex (const)
		const ::pthread_mutex_t& pthreadMutex() const;
		//@}
		# endif
		# endif


		//! \name Operators
		//@{
		//! Operator = (do nothing)
		Mutex& operator = (const Mutex&);
		# ifdef YUNI_HAS_CPP_MOVE
		// an OS's native mutex must have invariant address and thus can not be moved
		Mutex& operator = (Mutex&&) = delete;
		#endif
		//@}


	private:
		//! Destroy the current mutex
		inline void destroy();
		//! Create the mutex with settings from another mutex
		inline void copy(const Mutex& rhs);

	private:
		# ifndef YUNI_NO_THREAD_SAFE
		# ifdef YUNI_OS_WINDOWS
		//! The critical section
		CRITICAL_SECTION pSection;
		# else
		//! The PThread mutex
		::pthread_mutex_t pLock;
		::pthread_mutexattr_t pAttr;
		# endif
		# endif

	}; // class Mutex




	/*!
	** \brief Locks a mutex in the constructor and unlocks it in the destructor (RAII).
	**
	** This class is especially usefull for `get` accessor` and/or returned values
	** which have to be thread-safe.
	** This is a very common C++ idiom, known as "Resource Acquisition Is Initialization" (RAII).
	**
	** \code
	**	  class Foo
	**	  {
	**	  public:
	**		  Foo() : pValue(42) {}
	**		  ~Foo() {}
	**		  int getValue()
	**		  {
	**			  MutexLocker locker(pMutex);
	**			  return pValue;
	**		  }
	**		  void setValue(const int i)
	**		  {
	**			  pMutex.lock();
	**			  pValue = i;
	**			  pMutex.unlock();
	**		  }
	**	  private:
	**		  int pValue;
	**		  Mutex pMutex;
	**	  };
	** \endcode
	*/
	class MutexLocker final
	{
	public:
		//! \name Constructor & Destructor
		//@{
		/*!
		** \brief Constructor
		**
		** \param m The mutex to lock
		*/
		MutexLocker(Mutex& m);
		//! Destructor
		~MutexLocker();
		//@}

		MutexLocker& operator = (const MutexLocker&) = delete;

	private:
		//! Reference to the real mutex
		Mutex& pMutex;

	}; // MutexLocker




	//! All mutexes for each class
	template<class T> Mutex Mutex::ClassLevelLockable<T>::mutex;




} // namespace Yuni

# include "mutex.hxx"
