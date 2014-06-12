#ifndef JSONTEST_H_INCLUDED
# define JSONTEST_H_INCLUDED

# include <json/config.h>
# include <stdio.h>
# include <deque>
# include <string>

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// Mini Unit Testing framework
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////



/** \brief Unit testing framework.
 * \warning: all assertions are non-aborting, test case execution will continue
 *           even if an assertion namespace.
 *           This constraint is for portability: the framework needs to compile
 *           on Visual Studio 6 and must not require exception usage.
 */
namespace JsonTest {


   class Failure
   {
   public:
      const char *file_;
      unsigned int line_;
      std::string expr_;
      std::string message_;
      unsigned int nestingLevel_;
   };


   /// Context used to create the assertion callstack on failure.
   /// Must be a POD to allow inline initialisation without stepping 
   /// into the debugger.
   struct PredicateContext
   {
      typedef unsigned int Id;
      Id id_;
      const char *file_;
      unsigned int line_;
      const char *expr_;
      PredicateContext *next_;
      /// Related Failure, set when the PredicateContext is converted
      /// into a Failure.
      Failure *failure_;
   };

   class TestResult
   {
   public:
      TestResult();

      /// \internal Implementation detail for assertion macros
      /// Not encapsulated to prevent step into when debugging failed assertions
      /// Incremented by one on assertion predicate entry, decreased by one
      /// by addPredicateContext().
      PredicateContext::Id predicateId_;

      /// \internal Implementation detail for predicate macros
      PredicateContext *predicateStackTail_;

      void setTestName( const std::string &name );

      /// Adds an assertion failure.
      TestResult &addFailure( const char *file, unsigned int line,
                              const char *expr = 0 );

      /// Removes the last PredicateContext added to the predicate stack
      /// chained list.
      /// Next messages will be targed at the PredicateContext that was removed.
      TestResult &popPredicateContext();

      bool failed() const;

      void printFailure( bool printTestName ) const;

      TestResult &operator << ( bool value );
      TestResult &operator << ( int value );
      TestResult &operator << ( unsigned int value );
      TestResult &operator << ( double value );
      TestResult &operator << ( const char *value );
      TestResult &operator << ( const std::string &value );

   private:
      TestResult &addToLastFailure( const std::string &message );
      unsigned int getAssertionNestingLevel() const;
      /// Adds a failure or a predicate context
      void addFailureInfo( const char *file, unsigned int line,
                           const char *expr, unsigned int nestingLevel  );
      static std::string indentText( const std::string &text, 
                                     const std::string &indent );

      typedef std::deque<Failure> Failures;
      Failures failures_;
      std::string name_;
      PredicateContext rootPredicateNode_;
      PredicateContext::Id lastUsedPredicateId_;
      /// Failure which is the target of the messages added using operator <<
      Failure *messageTarget_;
   };


   class TestCase
   {
   public:
      TestCase();

      virtual ~TestCase();

      void run( TestResult &result );

      virtual const char *testName() const = 0;

   protected:
      TestResult *result_;

   private:
      virtual void runTestCase() = 0;
   };

   /// Function pointer type for TestCase factory
   typedef TestCase *(*TestCaseFactory)();

   class Runner
   {
   public:
      Runner();

      /// Adds a test to the suite
      Runner &add( TestCaseFactory factory );

      /// Runs test as specified on the command-line
      /// If no command-line arguments are provided, run all tests.
      /// If --list-tests is provided, then print the list of all test cases
      /// If --test <testname> is provided, then run test testname.
      int runCommandLine( int argc, const char *argv[] ) const;

      /// Runs all the test cases
      bool runAllTest( bool printSummary ) const;

      /// Returns the number of test case in the suite
      unsigned int testCount() const;

      /// Returns the name of the test case at the specified index
      std::string testNameAt( unsigned int index ) const;

      /// Runs the test case at the specified index using the specified TestResult
      void runTestAt( unsigned int index, TestResult &result ) const;

      static void printUsage( const char *appName );

   private: // prevents copy construction and assignment
      Runner( const Runner &other );
      Runner &operator =( const Runner &other );

   private:
      void listTests() const;
      bool testIndex( const std::string &testName, unsigned int &index ) const;
      static void preventDialogOnCrash();

   private:
      typedef std::deque<TestCaseFactory> Factories;
      Factories tests_;
   };

   template<typename T>
   TestResult &
   checkEqual( TestResult &result, const T &expected, const T &actual, 
               const char *file, unsigned int line, const char *expr )
   {
      if ( expected != actual )
      {
         result.addFailure( file, line, expr );
         result << "Expected: " << expected << "\n";
         result << "Actual  : " << actual;
      }
      return result;
   }

   TestResult &
   checkStringEqual( TestResult &result, 
                     const std::string &expected, const std::string &actual,
                     const char *file, unsigned int line, const char *expr );

} // namespace JsonTest


/// \brief Asserts that the given expression is true.
/// JSONTEST_ASSERT( x == y ) << "x=" << x << ", y=" << y;
/// JSONTEST_ASSERT( x == y );
#define JSONTEST_ASSERT( expr )                                               \
   if ( condition )                                                           \
   {                                                                          \
   }                                                                          \
   else                                                                       \
      result_->addFailure( __FILE__, __LINE__, #expr )

/// \brief Asserts that the given predicate is true.
/// The predicate may do other assertions and be a member function of the fixture.
#define JSONTEST_ASSERT_PRED( expr )                                    \
   {                                                                    \
      JsonTest::PredicateContext _minitest_Context = {                  \
         result_->predicateId_, __FILE__, __LINE__, #expr };            \
      result_->predicateStackTail_->next_ = &_minitest_Context;         \
      result_->predicateId_ += 1;                                       \
      result_->predicateStackTail_ = &_minitest_Context;                \
      (expr);                                                           \
      result_->popPredicateContext();                                   \
   }                                                                    \
   *result_

/// \brief Asserts that two values are equals.
#define JSONTEST_ASSERT_EQUAL( expected, actual )          \
   JsonTest::checkEqual( *result_, expected, actual,       \
                         __FILE__, __LINE__,               \
                         #expected " == " #actual )

/// \brief Asserts that two values are equals.
#define JSONTEST_ASSERT_STRING_EQUAL( expected, actual ) \
   JsonTest::checkStringEqual( *result_,                 \
      std::string(expected), std::string(actual),        \
      #expected " == " #actual )

/// \brief Begin a fixture test case.
#define JSONTEST_FIXTURE( FixtureType, name )                  \
   class Test##FixtureType##name : public FixtureType          \
   {                                                           \
   public:                                                     \
      static JsonTest::TestCase *factory()                     \
      {                                                        \
         return new Test##FixtureType##name();                 \
      }                                                        \
   public: /* overidden from TestCase */                       \
      virtual const char *testName() const                     \
      {                                                        \
         return #FixtureType "/" #name;                        \
      }                                                        \
      virtual void runTestCase();                              \
   };                                                          \
                                                               \
   void Test##FixtureType##name::runTestCase()

#define JSONTEST_FIXTURE_FACTORY( FixtureType, name ) \
   &Test##FixtureType##name::factory

#define JSONTEST_REGISTER_FIXTURE( runner, FixtureType, name ) \
   (runner).add( JSONTEST_FIXTURE_FACTORY( FixtureType, name ) )

#endif // ifndef JSONTEST_H_INCLUDED
