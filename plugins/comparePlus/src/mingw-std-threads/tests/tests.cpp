#include "../mingw.thread.h"
#include "../mingw.mutex.h"
#include "../mingw.condition_variable.h"
#include "../mingw.shared_mutex.h"
#include "../mingw.future.h"
#include <atomic>
#include <cassert>
#include <string>
#include <iostream>
#include <typeinfo>

using namespace std;

int test_int = 42;

//  Pre-declaration to suppress some warnings.
void test_call_once(int, char const *);

int cond = 0;
std::mutex m;
std::shared_mutex sm;
std::condition_variable cv;
std::condition_variable_any cv_any;

template<class ... Args>
void log (char const * fmtString, Args ...args) {
  printf(fmtString, args...);
  printf("\n");
  fflush(stdout);
}

void test_call_once(int a, const char* str)
{
    log("test_call_once called with a=%d, str=%s", a, str);
    this_thread::sleep_for(std::chrono::milliseconds(500));
}

struct TestMove
{
    std::string mStr;
    TestMove(const std::string& aStr): mStr(aStr){}
    TestMove(TestMove&& other): mStr(other.mStr+" moved")
    { printf("%s: Object moved\n", mStr.c_str()); }
    TestMove(const TestMove&) : mStr()
    {
        assert(false && "TestMove: Object COPIED instead of moved");
    }
};

template<class T>
void test_future_set_value (promise<T> & promise)
{
  promise.set_value(T(test_int));
}

template<>
void test_future_set_value (promise<void> & promise)
{
  promise.set_value();
}

template<class T>
bool test_future_get_value (future<T> & future)
{
  return (future.get() == T(test_int));
}

template<>
bool test_future_get_value (future<void> & future)
{
  future.get();
  return true;
}

template<class T>
struct CustomAllocator
{
  CustomAllocator (void) noexcept
  {
  }

  template<class U>
  CustomAllocator (CustomAllocator<U> const &) noexcept
  {
  }

  template<class U>
  CustomAllocator<T> & operator= (CustomAllocator<U> const &) noexcept
  {
    return *this;
  }

  typedef T value_type;
  T * allocate (size_t n)
  {
    log("Used custom allocator to allocate %zu object(s).", n);
    return static_cast<T*>(std::malloc(n * sizeof(T)));
  }
  void deallocate (T * ptr, size_t n)
  {
    log("Used custom allocator to deallocate %zu object(s).", n);
    std::free(ptr);
  }
};

template<class T>
void test_future ()
{
  static_assert(is_move_constructible<promise<T> >::value,
                "std::promise must be move-constructible.");
  static_assert(is_move_assignable<promise<T> >::value,
                "std::promise must be move-assignable.");
  static_assert(!is_copy_constructible<promise<T> >::value,
                "std::promise must not be copy-constructible.");
  static_assert(!is_copy_assignable<promise<T> >::value,
                "std::promise must not be copy-assignable.");

  static_assert(is_move_constructible<future<T> >::value,
                "std::future must be move-constructible.");
  static_assert(is_move_assignable<future<T> >::value,
                "std::future must be move-assignable.");
  static_assert(!is_copy_constructible<future<T> >::value,
                "std::future must not be copy-constructible.");
  static_assert(!is_copy_assignable<future<T> >::value,
                "std::future must not be copy-assignable.");

  static_assert(is_move_constructible<shared_future<T> >::value,
                "std::shared_future must be move-constructible.");
  static_assert(is_move_assignable<shared_future<T> >::value,
                "std::shared_future must be move-assignable.");
  static_assert(is_copy_constructible<shared_future<T> >::value,
                "std::shared_future must be copy-constructible.");
  static_assert(is_copy_assignable<shared_future<T> >::value,
                "std::shared_future must be copy-assignable.");

  log("\tMaking a few promises, and getting their futures...");
  promise<T> promise_value, promise_exception, promise_broken, promise_late;

  future<T> future_value = promise_value.get_future();
  future<T> future_exception = promise_exception.get_future();
  future<T> future_broken = promise_broken.get_future();
  future<T> future_late = promise_late.get_future();

  try {
    future<T> impossible_future = promise_value.get_future();
    log("WARNING: Promise failed to detect that its future was already retrieved.");
  } catch(...) {
    log("\tPromise successfully prevented redundant future retrieval.");
  }

  log("\tPassing promises to a new thread...");
  thread t ([](promise<T> p_value, promise<T> p_exception, promise<T>, promise<T> p_late)
    {
      this_thread::sleep_for(std::chrono::seconds(1));
      try {
        throw std::runtime_error("Thrown during the thread.");
      } catch (...) {
        p_late.set_exception_at_thread_exit(std::current_exception());
      }
      test_future_set_value(p_value);
      try {
        throw std::runtime_error("Things happened as expected.");
      } catch (...) {
        p_exception.set_exception(std::current_exception());
      }
      this_thread::sleep_for(std::chrono::seconds(2));
    },
    std::move(promise_value),
    std::move(promise_exception),
    std::move(promise_broken),
    std::move(promise_late));
  t.detach();

  try {
    bool was_expected = test_future_get_value(future_value);
    log("\tReceived %sexpected value.", (was_expected ? "" : "un"));
  } catch (...) {
    log("WARNING: Exception where there should be none!");
    throw;
  }
  try {
    test_future_get_value(future_exception);
    log("WARNING: Got a value where there should be an exception!");
  } catch (std::exception & e) {
    log("\tReceived an exception (\"%s\") as expected.", e.what());
  }

  log("\tWaiting for the thread to exit...");
  try {
    test_future_get_value(future_late);
    log("WARNING: Got a value where there should be an exception!");
  } catch (std::exception & e) {
    log("\tReceived an exception (\"%s\") as expected.", e.what());
  }

  try {
    test_future_get_value(future_broken);
    log("WARNING: Got a value where there should be an exception!");
  } catch (std::future_error & e) {
    log("\tReceived a future_error (\"%s\") as expected.", e.what());
  }

  log("\tDeferring a function...");
  auto async_deferred = async(launch::deferred, [] (void) -> T
    {
      std::hash<std::thread::id> hasher;
      log("\t\tDeferred function called on thread %zu", hasher(std::this_thread::get_id()));
      if (!is_void<T>::value)
        return T(test_int);
    });
  log("\tCalling a function asynchronously...");
  auto async_async = async(launch::async, [] (void) -> T
    {
      std::hash<std::thread::id> hasher;
      log("\t\tAsynchronous function called on thread %zu", hasher(std::this_thread::get_id()));
      if (!is_void<T>::value)
        return T(test_int);
    });
  log("\tLetting the implementation decide...");
  auto async_either = async([] (thread::id other_id) -> T
    {
      std::hash<thread::id> hasher;
      log("\t\tFunction called on thread %zu. Implementation chose %s execution.", hasher(this_thread::get_id()), (this_thread::get_id() == other_id) ? "deferred" : "asynchronous");
      if (!is_void<T>::value)
        return T(test_int);
    }, this_thread::get_id());

  log("\tFetching asynchronous result.");
  test_future_get_value(async_async);
  log("\tFetching deferred result.");
  test_future_get_value(async_deferred);
  log("\tFetching implementation-defined result.");
  test_future_get_value(async_either);

  log("\tTesting async on pointer-to-member-function.");
  struct Helper
  {
    thread::id other_id;
    T call (void) const
    {
      std::hash<thread::id> hasher;
      log("\t\tFunction called on thread %zu. Implementation chose %s execution.", hasher(this_thread::get_id()), (this_thread::get_id() == other_id) ? "deferred" : "asynchronous");
      if (!is_void<T>::value)
        return T(test_int);
    }
  } test_class { this_thread::get_id() };
  auto async_member = async(Helper::call, test_class);
  log("\tFetching result.");
  test_future_get_value(async_member);
}

#define TEST_SL_MV_CPY(ClassName) \
    static_assert(std::is_standard_layout<ClassName>::value, \
                  "ClassName does not satisfy concept StandardLayoutType."); \
    static_assert(!std::is_move_constructible<ClassName>::value, \
                  "ClassName must not be move-constructible."); \
    static_assert(!std::is_move_assignable<ClassName>::value, \
                  "ClassName must not be move-assignable."); \
    static_assert(!std::is_copy_constructible<ClassName>::value, \
                  "ClassName must not be copy-constructible."); \
    static_assert(!std::is_copy_assignable<ClassName>::value, \
                  "ClassName must not be copy-assignable.");

int main()
{
    TEST_SL_MV_CPY(mutex)
    TEST_SL_MV_CPY(recursive_mutex)
    TEST_SL_MV_CPY(timed_mutex)
    TEST_SL_MV_CPY(recursive_timed_mutex)
    TEST_SL_MV_CPY(shared_mutex)
    TEST_SL_MV_CPY(shared_timed_mutex)
    TEST_SL_MV_CPY(condition_variable)
    TEST_SL_MV_CPY(condition_variable_any)
    static_assert(!std::is_move_constructible<once_flag>::value,
                  "once_flag must not be move-constructible.");
    static_assert(!std::is_move_assignable<once_flag>::value,
                  "once_flag must not be move-assignable.");
    static_assert(!std::is_copy_constructible<once_flag>::value,
                  "once_flag must not be copy-constructible.");
    static_assert(!std::is_copy_assignable<once_flag>::value,
                  "once_flag must not be copy-assignable.");

//    With C++ feature level and target Windows version potentially affecting
//  behavior, make this information visible.
    {
        switch (__cplusplus)
        {
            case 201103L: std::cout << "Compiled in C++11"; break;
            case 201402L: std::cout << "Compiled in C++14"; break;
            case 201703L: std::cout << "Compiled in C++17"; break;
            default: std::cout << "Compiled in a non-conforming C++ compiler";
        }
        std::cout << ", targeting Windows ";
        static_assert(WINVER > 0x0500, "Windows NT and earlier are not supported.");
        switch (WINVER)
        {
            case 0x0501: std::cout << "XP"; break;
            case 0x0502: std::cout << "Server 2003"; break;
            case 0x0600: std::cout << "Vista"; break;
            case 0x0601: std::cout << "7"; break;
            case 0x0602: std::cout << "8"; break;
            case 0x0603: std::cout << "8.1"; break;
            case 0x0A00: std::cout << "10"; break;
            default: std::cout << "10+";
        }
        std::cout << "\n";
    }

    {
        log("Testing serialization and hashing for thread::id...");
        std::cout << "Serialization:\t" << this_thread::get_id() << "\n";
        std::hash<thread::id> hasher;
        std::cout << "Hash:\t" << hasher(this_thread::get_id()) << "\n";
    }
    std::thread t([](TestMove&& a, const char* b, int c) mutable
    {
        try
        {
            log("Worker thread started, sleeping for a while...");
//  Thread might move the string more than once.
            assert(a.mStr.substr(0, 15) == "move test moved");
            assert(!strcmp(b, "test message"));
            assert(c == -20);
            auto move2nd = std::move(a); //test move to final destination
            this_thread::sleep_for(std::chrono::milliseconds(1000));
            {
                lock_guard<mutex> lock(m);
                cond = 1;
                log("Notifying condvar");
                cv.notify_all();
            }

            this_thread::sleep_for(std::chrono::milliseconds(500));
            {
                lock_guard<decltype(sm)> lock(sm);
                cond = 2;
                log("Notifying condvar");
                cv_any.notify_all();
            }

            this_thread::sleep_for(std::chrono::milliseconds(500));
            {
                lock_guard<decltype(sm)> lock(sm);
                cond = 3;
                log("Notifying condvar");
                cv_any.notify_all();
            }

            log("Worker thread finishing");
        }
        catch(std::exception& e)
        {
            printf("EXCEPTION in worker thread: %s\n", e.what());
        }
    },
    TestMove("move test"), "test message", -20);
    try
    {
      log("Main thread: Locking mutex, waiting on condvar...");
      {
          std::unique_lock<decltype(m)> lk(m);
          cv.wait(lk, []{ return cond >= 1;} );
          log("condvar notified, cond = %d", cond);
          assert(lk.owns_lock());
      }
      log("Main thread: Locking shared_mutex, waiting on condvar...");
      {
          std::unique_lock<decltype(sm)> lk(sm);
          cv_any.wait(lk, []{ return cond >= 2;} );
          log("condvar notified, cond = %d", cond);
          assert(lk.owns_lock());
      }
      log("Main thread: Locking shared_mutex in shared mode, waiting on condvar...");
      {
          std::shared_lock<decltype(sm)> lk(sm);
          cv_any.wait(lk, []{ return cond >= 3;} );
          log("condvar notified, cond = %d", cond);
          assert(lk.owns_lock());
      }
      log("Main thread: Waiting on worker join...");

      t.join();
      log("Main thread: Worker thread joined");
      fflush(stdout);
    }
    catch(std::exception& e)
    {
        log("EXCEPTION in main thread: %s", e.what());
    }
    once_flag of;
    call_once(of, test_call_once, 1, "test");
    call_once(of, test_call_once, 1, "ERROR! Should not be called second time");
    log("Test complete");

    {
      log("Testing implementation of <future>...");
      test_future<int>();
      test_future<void>();
      test_future<int &>();
      test_future<int const &>();
      test_future<int volatile &>();
      test_future<int const volatile &>();
      log("Testing <future>'s use of allocators. Should allocate, then deallocate.");
      promise<int> allocated_promise (std::allocator_arg, CustomAllocator<unsigned>());
      allocated_promise.set_value(7);
    }

    return 0;
}
