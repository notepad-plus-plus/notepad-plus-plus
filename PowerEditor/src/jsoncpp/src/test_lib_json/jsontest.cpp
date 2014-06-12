#define _CRT_SECURE_NO_WARNINGS 1 // Prevents deprecation warning with MSVC
#include "jsontest.h"
#include <stdio.h>
#include <string>

#if defined(_MSC_VER)
// Used to install a report hook that prevent dialog on assertion and error.
# include <crtdbg.h>
#endif // if defined(_MSC_VER)

#if defined(_WIN32)
// Used to prevent dialog on memory fault.
// Limits headers included by Windows.h
# define WIN32_LEAN_AND_MEAN
# define NOSERVICE
# define NOMCX
# define NOIME
# define NOSOUND
# define NOCOMM
# define NORPC
# define NOGDI
# define NOUSER
# define NODRIVERS
# define NOLOGERROR
# define NOPROFILER
# define NOMEMMGR
# define NOLFILEIO
# define NOOPENFILE
# define NORESOURCE
# define NOATOM
# define NOLANGUAGE
# define NOLSTRING
# define NODBCS
# define NOKEYBOARDINFO
# define NOGDICAPMASKS
# define NOCOLOR
# define NOGDIOBJ
# define NODRAWTEXT
# define NOTEXTMETRIC
# define NOSCALABLEFONT
# define NOBITMAP
# define NORASTEROPS
# define NOMETAFILE
# define NOSYSMETRICS
# define NOSYSTEMPARAMSINFO
# define NOMSG
# define NOWINSTYLES
# define NOWINOFFSETS
# define NOSHOWWINDOW
# define NODEFERWINDOWPOS
# define NOVIRTUALKEYCODES
# define NOKEYSTATES
# define NOWH
# define NOMENUS
# define NOSCROLL
# define NOCLIPBOARD
# define NOICONS
# define NOMB
# define NOSYSCOMMANDS
# define NOMDI
# define NOCTLMGR
# define NOWINMESSAGES
# include <windows.h>
#endif // if defined(_WIN32)

namespace JsonTest {


// class TestResult
// //////////////////////////////////////////////////////////////////

TestResult::TestResult()
   : predicateId_( 1 )
   , lastUsedPredicateId_( 0 )
   , messageTarget_( 0 )
{
   // The root predicate has id 0
   rootPredicateNode_.id_ = 0;
   rootPredicateNode_.next_ = 0;
   predicateStackTail_ = &rootPredicateNode_;
}


void 
TestResult::setTestName( const std::string &name )
{
   name_ = name;
}

TestResult &
TestResult::addFailure( const char *file, unsigned int line,
                        const char *expr )
{
   /// Walks the PredicateContext stack adding them to failures_ if not already added.
   unsigned int nestingLevel = 0;
   PredicateContext *lastNode = rootPredicateNode_.next_;
   for ( ; lastNode != 0; lastNode = lastNode->next_ )
   {
      if ( lastNode->id_ > lastUsedPredicateId_ ) // new PredicateContext
      {
         lastUsedPredicateId_ = lastNode->id_;
         addFailureInfo( lastNode->file_, lastNode->line_, lastNode->expr_,
                         nestingLevel );
         // Link the PredicateContext to the failure for message target when 
         // popping the PredicateContext.
         lastNode->failure_ = &( failures_.back() );
      }
      ++nestingLevel;
   }

   // Adds the failed assertion
   addFailureInfo( file, line, expr, nestingLevel );
   messageTarget_ = &( failures_.back() );
   return *this;
}


void 
TestResult::addFailureInfo( const char *file, unsigned int line,
                            const char *expr, unsigned int nestingLevel )
{
   Failure failure;
   failure.file_ = file;
   failure.line_ = line;
   if ( expr )
   {
      failure.expr_ = expr;
   }
   failure.nestingLevel_ = nestingLevel;
   failures_.push_back( failure );
}


TestResult &
TestResult::popPredicateContext()
{
   PredicateContext *lastNode = &rootPredicateNode_;
   while ( lastNode->next_ != 0  &&  lastNode->next_->next_ != 0 )
   {
      lastNode = lastNode->next_;
   }
   // Set message target to popped failure
   PredicateContext *tail = lastNode->next_;
   if ( tail != 0  &&  tail->failure_ != 0 )
   {
      messageTarget_ = tail->failure_;
   }
   // Remove tail from list
   predicateStackTail_ = lastNode;
   lastNode->next_ = 0;
   return *this;
}


bool 
TestResult::failed() const
{
   return !failures_.empty();
}


unsigned int 
TestResult::getAssertionNestingLevel() const
{
   unsigned int level = 0;
   const PredicateContext *lastNode = &rootPredicateNode_;
   while ( lastNode->next_ != 0 )
   {
      lastNode = lastNode->next_;
      ++level;
   }
   return level;
}


void 
TestResult::printFailure( bool printTestName ) const
{
   if ( failures_.empty() )
   {
      return;
   }

   if ( printTestName )
   {
      printf( "* Detail of %s test failure:\n", name_.c_str() );
   }

   // Print in reverse to display the callstack in the right order
   Failures::const_iterator itEnd = failures_.end();
   for ( Failures::const_iterator it = failures_.begin(); it != itEnd; ++it )
   {
      const Failure &failure = *it;
      std::string indent( failure.nestingLevel_ * 2, ' ' );
      if ( failure.file_ )
      {
         printf( "%s%s(%d): ", indent.c_str(), failure.file_, failure.line_ );
      }
      if ( !failure.expr_.empty() )
      {
         printf( "%s\n", failure.expr_.c_str() );
      }
      else if ( failure.file_ )
      {
         printf( "\n" );
      }
      if ( !failure.message_.empty() )
      {
         std::string reindented = indentText( failure.message_, indent + "  " );
         printf( "%s\n", reindented.c_str() );
      }
   }
}


std::string 
TestResult::indentText( const std::string &text, 
                        const std::string &indent )
{
   std::string reindented;
   std::string::size_type lastIndex = 0;
   while ( lastIndex < text.size() )
   {
      std::string::size_type nextIndex = text.find( '\n', lastIndex );
      if ( nextIndex == std::string::npos )
      {
         nextIndex = text.size() - 1;
      }
      reindented += indent;
      reindented += text.substr( lastIndex, nextIndex - lastIndex + 1 );
      lastIndex = nextIndex + 1;
   }
   return reindented;
}


TestResult &
TestResult::addToLastFailure( const std::string &message )
{
   if ( messageTarget_ != 0 )
   {
      messageTarget_->message_ += message;
   }
   return *this;
}


TestResult &
TestResult::operator << ( bool value )
{
   return addToLastFailure( value ? "true" : "false" );
}


TestResult &
TestResult::operator << ( int value )
{
   char buffer[32];
   sprintf( buffer, "%d", value );
   return addToLastFailure( buffer );
}


TestResult &
TestResult::operator << ( unsigned int value )
{
   char buffer[32];
   sprintf( buffer, "%u", value );
   return addToLastFailure( buffer );
}


TestResult &
TestResult::operator << ( double value )
{
   char buffer[32];
   sprintf( buffer, "%16g", value );
   return addToLastFailure( buffer );
}


TestResult &
TestResult::operator << ( const char *value )
{
   return addToLastFailure( value ? value 
                                  : "<NULL>" );
}


TestResult &
TestResult::operator << ( const std::string &value )
{
   return addToLastFailure( value );
}



// class TestCase
// //////////////////////////////////////////////////////////////////

TestCase::TestCase()
   : result_( 0 )
{
}


TestCase::~TestCase()
{
}


void 
TestCase::run( TestResult &result )
{
   result_ = &result;
   runTestCase();
}



// class Runner
// //////////////////////////////////////////////////////////////////

Runner::Runner()
{
}


Runner &
Runner::add( TestCaseFactory factory )
{
   tests_.push_back( factory );
   return *this;
}


unsigned int 
Runner::testCount() const
{
   return static_cast<unsigned int>( tests_.size() );
}


std::string 
Runner::testNameAt( unsigned int index ) const
{
   TestCase *test = tests_[index]();
   std::string name = test->testName();
   delete test;
   return name;
}


void 
Runner::runTestAt( unsigned int index, TestResult &result ) const
{
   TestCase *test = tests_[index]();
   result.setTestName( test->testName() );
   printf( "Testing %s: ", test->testName() );
   fflush( stdout );
#if JSON_USE_EXCEPTION
   try 
   {
#endif // if JSON_USE_EXCEPTION
      test->run( result );
#if JSON_USE_EXCEPTION
   } 
   catch ( const std::exception &e ) 
   {
      result.addFailure( __FILE__, __LINE__, 
         "Unexpected exception caugth:" ) << e.what();
   }
#endif // if JSON_USE_EXCEPTION
   delete test;
   const char *status = result.failed() ? "FAILED" 
                                        : "OK";
   printf( "%s\n", status );
   fflush( stdout );
}


bool 
Runner::runAllTest( bool printSummary ) const
{
   unsigned int count = testCount();
   std::deque<TestResult> failures;
   for ( unsigned int index = 0; index < count; ++index )
   {
      TestResult result;
      runTestAt( index, result );
      if ( result.failed() )
      {
         failures.push_back( result );
      }
   }

   if ( failures.empty() )
   {
      if ( printSummary )
      {
         printf( "All %d tests passed\n", count );
      }
      return true;
   }
   else
   {
      for ( unsigned int index = 0; index < failures.size(); ++index )
      {
         TestResult &result = failures[index];
         result.printFailure( count > 1 );
      }

      if ( printSummary )
      {
         unsigned int failedCount = static_cast<unsigned int>( failures.size() );
         unsigned int passedCount = count - failedCount;
         printf( "%d/%d tests passed (%d failure(s))\n", passedCount, count, failedCount );
      }
      return false;
   }
}


bool 
Runner::testIndex( const std::string &testName, 
                   unsigned int &indexOut ) const
{
   unsigned int count = testCount();
   for ( unsigned int index = 0; index < count; ++index )
   {
      if ( testNameAt(index) == testName )
      {
         indexOut = index;
         return true;
      }
   }
   return false;
}


void 
Runner::listTests() const
{
   unsigned int count = testCount();
   for ( unsigned int index = 0; index < count; ++index )
   {
      printf( "%s\n", testNameAt( index ).c_str() );
   }
}


int 
Runner::runCommandLine( int argc, const char *argv[] ) const
{
   typedef std::deque<std::string> TestNames;
   Runner subrunner;
   for ( int index = 1; index < argc; ++index )
   {
      std::string opt = argv[index];
      if ( opt == "--list-tests" )
      {
         listTests();
         return 0;
      }
      else if ( opt == "--test-auto" )
      {
         preventDialogOnCrash();
      }
      else if ( opt == "--test" )
      {
         ++index;
         if ( index < argc )
         {
            unsigned int testNameIndex;
            if ( testIndex( argv[index], testNameIndex ) )
            {
               subrunner.add( tests_[testNameIndex] );
            }
            else
            {
               fprintf( stderr, "Test '%s' does not exist!\n", argv[index] );
               return 2;
            }
         }
         else
         {
            printUsage( argv[0] );
            return 2;
         }
      }
      else
      {
         printUsage( argv[0] );
         return 2;
      }
   }
   bool succeeded;
   if ( subrunner.testCount() > 0 )
   {
      succeeded = subrunner.runAllTest( subrunner.testCount() > 1 );
   }
   else
   {
      succeeded = runAllTest( true );
   }
   return succeeded ? 0 
                    : 1;
}


#if defined(_MSC_VER)
// Hook MSVCRT assertions to prevent dialog from appearing
static int 
msvcrtSilentReportHook( int reportType, char *message, int *returnValue )
{
   // The default CRT handling of error and assertion is to display
   // an error dialog to the user.
   // Instead, when an error or an assertion occurs, we force the 
   // application to terminate using abort() after display
   // the message on stderr. 
   if ( reportType == _CRT_ERROR  ||  
        reportType == _CRT_ASSERT )
   {
      // calling abort() cause the ReportHook to be called
      // The following is used to detect this case and let's the
      // error handler fallback on its default behaviour (
      // display a warning message)
      static volatile bool isAborting = false;
      if ( isAborting ) 
      {
         return TRUE;
      }
      isAborting = true;

      fprintf( stderr, "CRT Error/Assert:\n%s\n", message );
      fflush( stderr );
      abort();
   }
   // Let's other reportType (_CRT_WARNING) be handled as they would by default
   return FALSE;
}
#endif // if defined(_MSC_VER)


void 
Runner::preventDialogOnCrash()
{
#if defined(_MSC_VER)
   // Install a hook to prevent MSVCRT error and assertion from
   // popping a dialog.
   _CrtSetReportHook( &msvcrtSilentReportHook );
#endif // if defined(_MSC_VER)

   // @todo investiguate this handler (for buffer overflow)
   // _set_security_error_handler

#if defined(_WIN32)
   // Prevents the system from popping a dialog for debugging if the
   // application fails due to invalid memory access.
   SetErrorMode( SEM_FAILCRITICALERRORS 
                 | SEM_NOGPFAULTERRORBOX 
                 | SEM_NOOPENFILEERRORBOX );
#endif // if defined(_WIN32)
}

void 
Runner::printUsage( const char *appName )
{
   printf( 
      "Usage: %s [options]\n"
      "\n"
      "If --test is not specified, then all the test cases be run.\n"
      "\n"
      "Valid options:\n"
      "--list-tests: print the name of all test cases on the standard\n"
      "              output and exit.\n"
      "--test TESTNAME: executes the test case with the specified name.\n"
      "                 May be repeated.\n"
      "--test-auto: prevent dialog prompting for debugging on crash.\n"
      , appName );
}



// Assertion functions
// //////////////////////////////////////////////////////////////////

TestResult &
checkStringEqual( TestResult &result, 
                  const std::string &expected, const std::string &actual,
                  const char *file, unsigned int line, const char *expr )
{
   if ( expected != actual )
   {
      result.addFailure( file, line, expr );
      result << "Expected: '" << expected << "'\n";
      result << "Actual  : '" << actual << "'";
   }
   return result;
}


} // namespace JsonTest
