/// \file mingw.future.h
/// \brief Standard-compliant C++ futures for MinGW
///
/// (c) 2018 by Nathaniel J. McClatchey, San Jose, California
/// \author Nathaniel J. McClatchey, PhD
///
/// \copyright Simplified (2-clause) BSD License.
///
/// \note This file may become part of the mingw-w64 runtime package. If/when
/// this happens, the appropriate license will be added, i.e. this code will
/// become dual-licensed, and the current BSD 2-clause license will stay.
/// \note Target Windows version is determined by WINVER, which is determined in
/// <windows.h> from _WIN32_WINNT, which can itself be set by the user.

#ifndef MINGW_FUTURE_H_
#define MINGW_FUTURE_H_

#if !defined(__cplusplus) || (__cplusplus < 201103L)
#error The MinGW STD Threads library requires a compiler supporting C++11.
#endif

#include <future>

#include <cassert>
#include <vector>
#include <utility>        //  For std::pair
#include <type_traits>
#include <memory>
#include <functional>     //  For std::hash

#include "mingw.thread.h" //  Start new threads, and use invoke.

//  Mutexes and condition variables are used explicitly.
#include "mingw.mutex.h"
#include "mingw.condition_variable.h"

//  Note:
//    std::shared_ptr is the natural choice for this. However, a custom
//  implementation removes the need to keep a control block separate from the
//  class itself (no need to support weak pointers).

namespace mingw_stdthread
{
using std::future_errc;
using std::future_error;
using std::future_status;
using std::launch;
using std::future_category;

namespace detail
{
struct Empty { };

//    Use a class template to allow instantiation of statics in a header-only
//  library. Note: Template will only be instantiated once to avoid bloat.
template<bool>
struct FutureStatic
{
  enum Type : uint_fast8_t
  {
    kUndecided = 0x00,
    kDeferred = 0x05,
    kValue = 0x02,
    kException = 0x03,
    kSetFlag = 0x02,
    kTypeMask = 0x03,
    kReadyFlag = 0x04
  };

  static std::vector<std::pair<mutex, condition_variable> > sync_pool;

  static mutex & get_mutex (void const * ptr)
  {
    std::hash<void const *> hash_func;
    return sync_pool[hash_func(ptr) % sync_pool.size()].first;
  }
  static condition_variable & get_condition_variable (void const * ptr)
  {
    std::hash<void const *> hash_func;
    return sync_pool[hash_func(ptr) % sync_pool.size()].second;
  }
};
template<bool b>
std::vector<std::pair<mutex, condition_variable> > FutureStatic<b>::sync_pool (thread::hardware_concurrency() * 2 + 1);

struct FutureStateBase
{
  inline mutex & get_mutex (void) const
  {
    return FutureStatic<true>::get_mutex(this);
  }
  inline condition_variable & get_condition_variable (void) const
  {
    return FutureStatic<true>::get_condition_variable(this);
  }
  typedef typename FutureStatic<true>::Type Type;
//  Destroys this object. Used for allocator-awareness.
  virtual void deallocate_this (void) noexcept = 0;
  virtual ~FutureStateBase (void) = default;

  FutureStateBase (FutureStateBase const &) = delete;
  FutureStateBase & operator= (FutureStateBase const &) = delete;

  FutureStateBase(Type t) noexcept
    : mReferences(0), mType(t)
  {
  }

  void increment_references (void) noexcept
  {
    mReferences.fetch_add(1, std::memory_order_relaxed);
  }

  void decrement_references (void) noexcept
  {
    if (mReferences.fetch_sub(1, std::memory_order_acquire) == 0)
      deallocate_this();
  }

  std::atomic<size_t> mReferences;
  std::atomic<uint_fast8_t> mType;
};

//  Reduce compilation time and improve code re-use.
struct FutureBase : public FutureStatic<true>
{
  typedef FutureStatic<true> Base;
  FutureStateBase * mState;

  mutex & get_mutex (void) const
  {
    return FutureStatic<true>::get_mutex(mState);
  }
  condition_variable & get_condition_variable (void) const
  {
    return FutureStatic<true>::get_condition_variable(mState);
  }

  FutureBase (FutureStateBase * ptr) noexcept
    : mState(ptr)
  {
  }

  FutureBase (FutureBase && source) noexcept
    : mState(source.mState)
  {
    source.mState = nullptr;
  }

  ~FutureBase (void)
  {
    release();
  }

  FutureBase (FutureBase const &) = delete;
  FutureBase & operator= (FutureBase const &) = delete;

  bool valid (void) const noexcept
  {
    return mState != nullptr;
  }

//    Releases this object's hold on its state. Requires a specification of
//  which state is being used.
  inline void release (void) noexcept
  {
    if (valid())
      mState->decrement_references();
    mState = nullptr;
  }

  void wait (std::unique_lock<mutex> & lock) const
  {
#if !defined(NDEBUG)
    if (!valid())
      throw future_error(future_errc::no_state);
#endif
//    If there's already a value or exception, don't do any extraneous
//  synchronization. The `get()` method will do that for us.
    if (mState->mType.load(std::memory_order_relaxed) & kReadyFlag)
      return;
    get_condition_variable().wait(lock, [this](void)->bool {
      return mState->mType.load(std::memory_order_relaxed) & kReadyFlag;
    });
  }

  template<class Rep, class Period>
  future_status wait_for (std::chrono::duration<Rep,Period> const & dur) const
  {
#if !defined(NDEBUG)
    if (!valid())
      throw future_error(future_errc::no_state);
#endif
    auto current_state = mState->mType.load(std::memory_order_relaxed);
    if (current_state & kReadyFlag)
      return (current_state == kDeferred) ? future_status::deferred : future_status::ready;
    std::unique_lock<mutex> lock { get_mutex() };
    if (get_condition_variable().wait_for(lock, dur,
          [this](void)->bool {
            return mState->mType.load(std::memory_order_relaxed) & kReadyFlag;
          }))
      return future_status::ready;
    else
      return future_status::timeout;
  }

  template<class Clock, class Duration>
  future_status wait_until(const std::chrono::time_point<Clock,Duration>& time) const
  {
    return wait_for(time - Clock::now());
  }
};

template<class T>
struct FutureState : public FutureStateBase
{
//    The state never needs more than one of these at any one time, so don't
//  waste space or allocation time.
  union {
    struct {} mUndecided;  //  Included to make the active member unambiguous.
    T mObject;
    std::exception_ptr mException;
    std::function<void(void)> mFunction;
  };

  FutureState (void) noexcept
    : FutureStateBase(Type::kUndecided), mUndecided()
  {
  }

  FutureState (std::function<void(void)> && deferred_function)
    : FutureStateBase(Type::kDeferred), mFunction(std::move(deferred_function))
  {
  }

  void deallocate_this (void) noexcept override
  {
    delete this;
  }

  template<class Arg>
  void set_value (Arg && arg)
  {
    assert(!(mType.load(std::memory_order_relaxed) & Type::kSetFlag));
    new(&mObject) T (std::forward<Arg>(arg));
    mType.store(Type::kValue | Type::kReadyFlag, std::memory_order_release);
  }
  template<class Arg>
  void set_exception (Arg && arg)
  {
    assert(!(mType.load(std::memory_order_relaxed) & Type::kSetFlag));
    new(&mException) std::exception_ptr (std::forward<Arg>(arg));
    mType.store(Type::kException | Type::kReadyFlag, std::memory_order_release);
  }
//  These overloads set value/exception, but don't make it ready.
  template<class Arg>
  void set_value (Arg && arg, bool)
  {
    assert(!(mType.load(std::memory_order_relaxed) & Type::kSetFlag));
    new(&mObject) T (std::forward<Arg>(arg));
    mType.store(Type::kValue, std::memory_order_release);
  }
  template<class Arg>
  void set_exception (Arg && arg, bool)
  {
    assert(!(mType.load(std::memory_order_relaxed) & Type::kSetFlag));
    new(&mException) std::exception_ptr (std::forward<Arg>(arg));
    mType.store(Type::kException, std::memory_order_release);
  }
 //private:
  ~FutureState (void)
  {
    switch (mType.load(std::memory_order_acquire) & Type::kTypeMask)
    {
    case Type::kDeferred & Type::kTypeMask:
      mFunction.~function();
      break;
    case Type::kValue:
      mObject.~T();
      break;
    case Type::kException:
      mException.~exception_ptr();
      break;
    default:;
    }
  }
};

template<class T, class Alloc>
struct FutureStateAllocated : public FutureState<T>
{
  typedef typename std::allocator_traits<Alloc>::void_pointer void_pointer;
  void_pointer mThis;
  Alloc mAllocator;

  FutureStateAllocated (Alloc const & alloc, void_pointer const & vptr) noexcept
    : FutureState<T>(), mThis(vptr), mAllocator(alloc)
  {
  }

  FutureStateAllocated (FutureStateAllocated<T,Alloc> const &) = delete;
  FutureStateAllocated<T,Alloc> & operator= (FutureStateAllocated<T,Alloc> const &) = delete;

  void deallocate_this (void) noexcept override
  {
    typedef typename std::allocator_traits<Alloc>::template rebind_traits<FutureStateAllocated<T, Alloc> > allocator_traits;
    typename allocator_traits::allocator_type alloc(std::move(mAllocator));
    typedef typename allocator_traits::pointer pointer;
    pointer ptr(static_cast<pointer>(mThis));
    allocator_traits::destroy(alloc, this);
    allocator_traits::deallocate(alloc, ptr, 1);
  }
};
} //  Namespace "detail"

#if (defined(__MINGW32__ ) && !defined(_GLIBCXX_HAS_GTHREADS))
}
namespace std {
#else
template<class T>
class future;
template<class T>
class shared_future;
template<class T>
class promise;
#endif

template<class T>
class future : mingw_stdthread::detail::FutureBase
{
  typedef mingw_stdthread::detail::FutureState<T> state_type;
  future (state_type * ptr) noexcept
    : FutureBase(ptr)
  {
  }

  friend class shared_future<T>;
  friend class promise<T>;

  template<class U>
  friend class future;

  template<class _Fn, class ... _Args>
  friend future<__async_result_of<_Fn, _Args...>> async (std::launch, _Fn &&, _Args&&...);
 public:
  using FutureBase::valid;
  using FutureBase::wait_for;
  using FutureBase::wait_until;

  future (void) noexcept
    : FutureBase(nullptr)
  {
  }
  future<T> & operator= (future<T> && source) noexcept
  {
//  Check for this atypical behavior rather than creating a nonsensical state.
    if (this != &source)
    {
      release();
      mState = source.mState;
      source.mState = nullptr;
    }
    return *this;
  }
  future (future<T> && source) noexcept
    : FutureBase(std::move(source))
  {
  }

  ~future (void) = default;

  future (future<T> const &) = delete;
  future<T> & operator= (future<T> const &) = delete;

  T const & get (void) const
  {
    wait();
    if (mState->mType.load(std::memory_order_acquire) == (kValue | kReadyFlag))
      return static_cast<state_type *>(mState)->mObject;
    else
    {
      assert(mState->mType.load(std::memory_order_relaxed) == (kException | kReadyFlag));
      std::rethrow_exception(static_cast<state_type *>(mState)->mException);
    }
  }

  shared_future<T> share (void) noexcept;

  void wait (void) const
  {
    std::unique_lock<mingw_stdthread::mutex> lock { get_mutex() };
    FutureBase::wait(lock);
    if (mState->mType.load(std::memory_order_acquire) == kDeferred)
    {
      state_type * ptr = static_cast<state_type *>(mState);
      decltype(ptr->mFunction) func = std::move(ptr->mFunction);
      ptr->mFunction.~function();
      func();
      ptr->get_condition_variable().notify_all();
    }
  }
};

template<class T>
class shared_future : future<T>
{
  typedef typename future<T>::state_type state_type;
 public:
  using future<T>::get;
  using future<T>::wait;
  using future<T>::wait_for;
  using future<T>::wait_until;
  using future<T>::valid;

  shared_future (void) noexcept : future<T>()
  {
  }

  shared_future (shared_future<T> && source) noexcept
    : future<T>(std::move(source))
  {
  }

  shared_future<T> & operator= (shared_future<T> && source) noexcept
  {
    return future<T>::operator=(std::move(source));
  }

  shared_future (shared_future<T> const & source) noexcept(__cplusplus >= 201703L)
    : future<T>(static_cast<state_type *>(source.mState))
  {
    future<T>::mState->increment_references();
  }

  shared_future<T> & operator= (shared_future<T> const & source) noexcept(__cplusplus >= 201703L)
  {
    if (future<T>::mState == source.mState)
      return *this;
    future<T>::release();
    future<T>::mState = source.mState;
    future<T>::mState->increment_references();
    return *this;
  }

  shared_future (future<T> && source) noexcept
    : future<T>(std::move(source))
  {
  }

  shared_future<T> & operator= (future<T> && source) noexcept
  {
    future<T>::operator=(std::move(source));
    return *this;
  }

  ~shared_future (void) = default;
};

template<class T>
class promise : mingw_stdthread::detail::FutureBase
{
  bool mRetrieved;
  typedef mingw_stdthread::detail::FutureState<T> state_type;
  void check_before_set (void) const
  {
    if (!valid())
      throw future_error(future_errc::no_state);
    if (mState->mType.load(std::memory_order_relaxed) & kSetFlag)
      throw future_error(future_errc::promise_already_satisfied);
  }

  void check_abandon (void)
  {
    if (valid() && !(mState->mType.load(std::memory_order_relaxed) & kSetFlag))
    {
      try {
        throw future_error(future_errc::broken_promise);
      } catch (...) {
        set_exception(std::current_exception());
      }
    }
  }
/// \bug Might throw more exceptions than specified by the standard...
//  Need OS support for this...
  void make_ready_at_thread_exit (void)
  {
//  Need to turn the pseudohandle from GetCurrentThread() into a true handle...
    HANDLE thread_handle;
    BOOL success = DuplicateHandle(GetCurrentProcess(),
                                   GetCurrentThread(),
                                   GetCurrentProcess(),
                                   &thread_handle,
                                   0, //  Access doesn't matter. Will be duplicated.
                                   FALSE, //  No need for this to be inherited.
                                   DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    if (!success)
      throw std::runtime_error("MinGW STD Threads library failed to make a promise ready after thread exit.");

    mState->increment_references();
    bool handle_handled = false;
    try {
      state_type * ptr = static_cast<state_type *>(mState);
      mingw_stdthread::thread watcher_thread ([ptr, thread_handle, &handle_handled](void)
        {
          {
            std::lock_guard<mingw_stdthread::mutex> guard (ptr->get_mutex());
            handle_handled = true;
          }
          ptr->get_condition_variable().notify_all();
//  Wait for the original thread to die.
          WaitForSingleObject(thread_handle, INFINITE);
          CloseHandle(thread_handle);

          {
            std::lock_guard<mingw_stdthread::mutex> guard (ptr->get_mutex());
            ptr->mType.fetch_or(kReadyFlag, std::memory_order_relaxed);
          }
          ptr->get_condition_variable().notify_all();

          ptr->decrement_references();
        });
      {
        std::unique_lock<mingw_stdthread::mutex> guard (ptr->get_mutex());
        ptr->get_condition_variable().wait(guard, [&handle_handled](void)->bool
          {
            return handle_handled;
          });
      }
      watcher_thread.detach();
    }
    catch (...)
    {
//    Because the original promise is still alive, this can't be the decrement
//  destroys it.
      mState->decrement_references();
      if (!handle_handled)
        CloseHandle(thread_handle);
    }
  }

  template<class U>
  future<U> make_future (void)
  {
    if (!valid())
      throw future_error(future_errc::no_state);
    if (mRetrieved)
      throw future_error(future_errc::future_already_retrieved);
    mState->increment_references();
    mRetrieved = true;
    return future<U>(static_cast<state_type *>(mState));
  }

  template<class U>
  friend class promise;
 public:
//    Create a promise with an empty state, with the reference counter set to
//  indicate that the state is only held by this promise (i.e. not by any
//  futures).
  promise (void)
    : FutureBase(new state_type ()), mRetrieved(false)
  {
  }

  template<typename Alloc>
  promise (std::allocator_arg_t, Alloc const & alloc)
    : FutureBase(nullptr), mRetrieved(false)
  {
    typedef mingw_stdthread::detail::FutureStateAllocated<T,Alloc> State;
    typedef typename std::allocator_traits<Alloc>::template rebind_traits<State> Traits;
    typename Traits::allocator_type rebound_alloc(alloc);
    typename Traits::pointer ptr = Traits::allocate(rebound_alloc, 1);
    typename Traits::void_pointer vptr = ptr;
    State * sptr = std::addressof(*ptr);
    Traits::construct(rebound_alloc, sptr, std::move(rebound_alloc), vptr);
    mState = static_cast<state_type *>(sptr);
  }

  promise (promise<T> && source) noexcept
    : FutureBase(std::move(source)), mRetrieved(source.mRetrieved)
  {
  }

  ~promise (void)
  {
    check_abandon();
  }

  promise<T> & operator= (promise<T> && source) noexcept
  {
    if (this == &source)
      return *this;
    check_abandon();
    release();
    mState = source.mState;
    mRetrieved = source.mRetrieved;
    source.mState = nullptr;
    return *this;
  }

  void swap (promise<T> & other) noexcept
  {
    std::swap(mState, other.mState);
    std::swap(mRetrieved, other.mRetrieved);
  }

  promise (promise<T> const &) = delete;
  promise<T> & operator= (promise<T> const &) = delete;

  future<T> get_future (void)
  {
    return make_future<T>();
  }

  void set_value (T const & value)
  {
    {
      std::lock_guard<mingw_stdthread::mutex> lock { get_mutex() };
      check_before_set();
      static_cast<state_type *>(mState)->set_value(value);
    }
    get_condition_variable().notify_all();
  }

  void set_value (T && value)
  {
    {
      std::lock_guard<mingw_stdthread::mutex> lock { get_mutex() };
      check_before_set();
      static_cast<state_type *>(mState)->set_value(std::move(value));
    }
    get_condition_variable().notify_all();
  }

  void set_value_at_thread_exit (T const & value)
  {
    {
      std::lock_guard<mingw_stdthread::mutex> lock { get_mutex() };
      check_before_set();
      static_cast<state_type *>(mState)->set_value(value, false);
    }
    make_ready_at_thread_exit();
  }

  void set_value_at_thread_exit (T && value)
  {
    {
      std::lock_guard<mingw_stdthread::mutex> lock { get_mutex() };
      check_before_set();
      static_cast<state_type *>(mState)->set_value(std::move(value), false);
    }
    make_ready_at_thread_exit();
  }

  void set_exception (std::exception_ptr eptr)
  {
    {
      std::lock_guard<mingw_stdthread::mutex> lock { get_mutex() };
      check_before_set();
      static_cast<state_type *>(mState)->set_exception(eptr);
    }
    get_condition_variable().notify_all();
  }

  void set_exception_at_thread_exit (std::exception_ptr eptr)
  {
    {
      std::lock_guard<mingw_stdthread::mutex> lock { get_mutex() };
      check_before_set();
      static_cast<state_type *>(mState)->set_exception(eptr, false);
    }
    make_ready_at_thread_exit();
  }
};

////////////////////////////////////////////////////////////////////////////////
//                           Reference Specialization                         //
////////////////////////////////////////////////////////////////////////////////

template<class T>
class future<T&> : future<void *>
{
  typedef future<void *> Base;
  template<class U>
  friend class shared_future;
  template<class U>
  friend class promise;

  future (typename Base::state_type * state)
    : Base(state)
  {
  }

  template<class _Fn, class ... _Args>
  friend future<__async_result_of<_Fn, _Args...>> async (std::launch, _Fn &&, _Args&&...);
 public:
  using Base::valid;
  using Base::wait_for;
  using Base::wait_until;
  using Base::wait;

  future (void) noexcept = default;

  inline T& get (void) const
  {
    return *static_cast<T *>(Base::get());
  }

  shared_future<T&> share (void) noexcept;
};

template<class T>
class shared_future<T&> : shared_future<void *>
{
  typedef shared_future<void *> Base;
 public:
  using Base::wait;
  using Base::wait_for;
  using Base::wait_until;
  using Base::valid;

  inline T& get (void) const
  {
    return *static_cast<T *>(Base::get());
  }

  shared_future (future<T&> && source) noexcept
    : Base(std::move(source))
  {
  }

  shared_future<T&> & operator= (future<T&> && source) noexcept
  {
    Base::operator=(std::move(source));
    return *this;
  }

  ~shared_future (void) = default;
};

template<class T>
class promise<T&> : private promise<void *>
{
  typedef promise<void *> Base;
 public:
  using Base::set_exception;
  using Base::set_exception_at_thread_exit;

  promise (void) = default;
  template<typename Alloc>
  promise (std::allocator_arg_t arg, Alloc const & alloc)
    : Base(arg, alloc)
  {
  }

  inline void set_value (T & value)
  {
    typedef typename std::remove_cv<T>::type T_non_cv;
    Base::set_value(const_cast<T_non_cv *>(std::addressof(value)));
  }

  inline void set_value_at_thread_exit (T & value)
  {
    typedef typename std::remove_cv<T>::type T_non_cv;
    Base::set_value_at_thread_exit(const_cast<T_non_cv *>(std::addressof(value)));
  }

  inline future<T&> get_future (void)
  {
    return Base::template make_future<T&>();
  }

  void swap (promise<T&> & other) noexcept
  {
    Base::swap(other);
  }
};

////////////////////////////////////////////////////////////////////////////////
//                              Void Specialization                           //
////////////////////////////////////////////////////////////////////////////////

template<>
class future<void> : future<mingw_stdthread::detail::Empty>
{
  typedef mingw_stdthread::detail::Empty Empty;
  template<class U>
  friend class shared_future;
  template<class U>
  friend class promise;

  future(future<Empty>::state_type * state)
    : future<Empty>(state)
  {
  }

  template<class _Fn, class ... _Args>
  friend future<__async_result_of<_Fn, _Args...>> async (std::launch, _Fn &&, _Args&&...);

 public:
  using future<Empty>::valid;
  using future<Empty>::wait_for;
  using future<Empty>::wait_until;
  using future<Empty>::wait;

  future (void) noexcept = default;

  void get (void) const
  {
    future<Empty>::get();
  }

  shared_future<void> share (void) noexcept;
};

template<>
class shared_future<void> : shared_future<mingw_stdthread::detail::Empty>
{
  typedef mingw_stdthread::detail::Empty Empty;
 public:
  using shared_future<Empty>::wait;
  using shared_future<Empty>::wait_for;
  using shared_future<Empty>::wait_until;
  using shared_future<Empty>::valid;

  void get (void) const
  {
    shared_future<Empty>::get();
  }

  shared_future (void) noexcept = default;

  shared_future (shared_future<void> && source) noexcept = default;

  shared_future<void> & operator= (shared_future<void> && source) noexcept = default;

  shared_future (shared_future<void> const & source) noexcept(__cplusplus >= 201703L) = default;

  shared_future<void> & operator= (shared_future<void> const & source) noexcept(__cplusplus >= 201703L) = default;

  shared_future (future<void> && source) noexcept
    : shared_future<Empty>(std::move(source))
  {
  }

  shared_future<void> & operator= (future<void> && source) noexcept
  {
    shared_future<Empty>::operator=(std::move(source));
    return *this;
  }

  ~shared_future (void) = default;
};

inline shared_future<void> future<void>::share (void) noexcept
{
  return shared_future<void>(std::move(*this));
}

template<class T>
shared_future<T> future<T>::share (void) noexcept
{
  return shared_future<T>(std::move(*this));
}

template<class T>
shared_future<T&> future<T&>::share (void) noexcept
{
  return shared_future<T&>(std::move(*this));
}

template<>
class promise<void> : private promise<mingw_stdthread::detail::Empty>
{
  typedef mingw_stdthread::detail::Empty Empty;
 public:
  using promise<Empty>::set_exception;
  using promise<Empty>::set_exception_at_thread_exit;

  promise (void) = default;
  template<typename Alloc>
  promise (std::allocator_arg_t arg, Alloc const & alloc)
    : promise<Empty>(arg, alloc)
  {
  }

  inline void set_value (void)
  {
    promise<Empty>::set_value(Empty());
  }

  inline void set_value_at_thread_exit (void)
  {
    promise<Empty>::set_value_at_thread_exit(Empty());
  }

  inline future<void> get_future (void)
  {
    return promise<Empty>::template make_future<void>();
  }

  void swap (promise<void> & other) noexcept
  {
    promise<Empty>::swap(other);
  }
};



template<class T>
void swap(promise<T> & lhs, promise<T> & rhs) noexcept
{
  lhs.swap(rhs);
}

template<class T, class Alloc>
struct uses_allocator<promise<T>, Alloc> : std::true_type
{
};

} //  Namespace "std"
namespace mingw_stdthread
{
namespace detail
{
template<class Ret>
struct StorageHelper
{
  template<class Func, class ... Args>
  static void store_deferred (FutureState<Ret> * state_ptr, Func && func, Args&&... args)
  {
    try {
      state_ptr->set_value(invoke(std::forward<Func>(func), std::forward<Args>(args)...));
    } catch (...) {
      state_ptr->set_exception(std::current_exception());
    }
  }
  template<class Func, class ... Args>
  static void store (FutureState<Ret> * state_ptr, Func && func, Args&&... args)
  {
    {
      std::lock_guard<mingw_stdthread::mutex> lock { state_ptr->get_mutex() };
      store_deferred(state_ptr, std::forward<Func>(func), std::forward<Args>(args)...);
    }
    state_ptr->get_condition_variable().notify_all();
  }
};

template<class Ref>
struct StorageHelper<Ref&>
{
  template<class Func, class ... Args>
  static void store_deferred (FutureState<void*> * state_ptr, Func && func, Args&&... args)
  {
    try {
      typedef typename std::remove_cv<Ref>::type Ref_non_cv;
      Ref & rf = invoke(std::forward<Func>(func), std::forward<Args>(args)...);
      state_ptr->set_value(const_cast<Ref_non_cv *>(std::addressof(rf)));
    } catch (...) {
      state_ptr->set_exception(std::current_exception());
    }
  }
  template<class Func, class ... Args>
  static void store (FutureState<void*> * state_ptr, Func && func, Args&&... args)
  {
    {
      std::lock_guard<mingw_stdthread::mutex> lock { state_ptr->get_mutex() };
      store_deferred(state_ptr, std::forward<Func>(func), std::forward<Args>(args)...);
    }
    state_ptr->get_condition_variable().notify_all();
  }
};

template<>
struct StorageHelper<void>
{
  template<class Func, class ... Args>
  static void store_deferred (FutureState<Empty> * state_ptr, Func && func, Args&&... args)
  {
    try {
      invoke(std::forward<Func>(func), std::forward<Args>(args)...);
      state_ptr->set_value(Empty{});
    } catch (...) {
      state_ptr->set_exception(std::current_exception());
    }
  }
  template<class Func, class ... Args>
  static void store (FutureState<Empty> * state_ptr, Func && func, Args&&... args)
  {
    {
      std::lock_guard<mingw_stdthread::mutex> lock { state_ptr->get_mutex() };
      store_deferred(state_ptr, std::forward<Func>(func), std::forward<Args>(args)...);
    }
    state_ptr->get_condition_variable().notify_all();
  }
};
} //  Namespace "detail"
} //  Namespace "mingw_stdthread"
namespace std
{


//    Unfortunately, MinGW's <future> locks us into a particular (non-standard)
//  signature for async.
template< class Function, class... Args>
/*#if (__cplusplus < 201703L)
std::future<std::result_of<std::decay<Function>::type(std::decay<Args>::type...)>::type>
#else
#if (__cplusplus > 201703L)
[[nodiscard]]
#endif
std::future<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>>
#endif*/
#if (__cplusplus > 201703L)
[[nodiscard]]
#endif
std::future<__async_result_of<Function, Args...> >
  async(Function&& f, Args&&... args)
{
  return async(launch::async | launch::deferred, std::forward<Function>(f), std::forward<Args>(args)...);
}
template< class Function, class... Args >
/*#if (__cplusplus < 201703L)
std::future<std::result_of<std::decay<Function>::type(std::decay<Args>::type...)>::type>
#else
#if (__cplusplus > 201703L)
[[nodiscard]]
#endif
std::future<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...> >
#endif*/
#if (__cplusplus > 201703L)
[[nodiscard]]
#endif
std::future<__async_result_of<Function, Args...> >
  async(std::launch policy, Function&& f, Args&&... args)
{
  typedef __async_result_of<Function, Args...> result_type;
/*#if (__cplusplus < 201703L)
  typedef std::result_of<std::decay<Function>::type(std::decay<Args>::type...)>::type result_type;
#else
  typedef std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...> result_type;
#endif*/
  typedef future<result_type> future_type;
  typedef typename future_type::state_type state_type;

  //auto setter = []

  state_type * state_ptr = nullptr;
  /*if ((policy & std::launch::async) == std::launch::async)
    state_ptr = new state_type ();
  else
    state_ptr = new state_type (std::function<result_type(void)>(std::bind(std::forward<Function>(f), std::forward<Args>(args)...)));*/


  if ((policy & std::launch::async) == std::launch::async)
  {
    auto deleter = [](state_type * ptr) { ptr->decrement_references(); };
    state_ptr = new state_type ();
    state_ptr->increment_references();
    std::unique_ptr<state_type, decltype(deleter)> ooptr { state_ptr, deleter };
    mingw_stdthread::thread t ([](decltype(ooptr) ptr, typename std::decay<Function>::type f2, typename std::decay<Args>::type... args2)
      {
        typedef mingw_stdthread::detail::StorageHelper<result_type> s_helper;
        s_helper::store(ptr.get(), f2, args2...);
      }, std::move(ooptr), std::forward<Function>(f), std::forward<Args>(args)...);
    t.detach();
  } else {
    typedef std::function<result_type(void)> func_type;
    struct Packed
    {
      func_type func;
      state_type * ptr;
    };
    std::shared_ptr<Packed>  bound { new Packed { std::bind(std::forward<Function>(f), std::forward<Args>(args)...), nullptr } };
    state_ptr = new state_type (std::function<void(void)>([bound](void)
      {
        typedef mingw_stdthread::detail::StorageHelper<result_type> s_helper;
        s_helper::store_deferred(bound->ptr, std::move(bound->func));
      }));
    bound->ptr = state_ptr;
  }
  assert(state_ptr != nullptr);
  return future<result_type> { state_ptr };
}

#if (defined(__MINGW32__ ) && !defined(_GLIBCXX_HAS_GTHREADS))
} //  Namespace std
namespace mingw_stdthread
{
using std::future;
using std::shared_future;
using std::promise;
using std::async;
#else
} //  Namespace mingw_stdthread
namespace std
{
template<class T>
void swap(mingw_stdthread::promise<T> & lhs, mingw_stdthread::promise<T> & rhs) noexcept
{
  lhs.swap(rhs);
}

template<class T, class Alloc>
struct uses_allocator<mingw_stdthread::promise<T>, Alloc> : std::true_type
{
};
#endif
} //  Namespace

#endif // MINGW_FUTURE_H_
