#include <json/json.h>
#include "jsontest.h"


// TODO:
// - boolean value returns that they are integral. Should not be.
// - unsigned integer in integer range are not considered to be valid integer. Should check range.


// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// Json Library test cases
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////


struct ValueTest : JsonTest::TestCase
{
   Json::Value null_;
   Json::Value emptyArray_;
   Json::Value emptyObject_;
   Json::Value integer_;
   Json::Value unsignedInteger_;
   Json::Value smallUnsignedInteger_;
   Json::Value real_;
   Json::Value array1_;
   Json::Value object1_;
   Json::Value emptyString_;
   Json::Value string1_;
   Json::Value string_;
   Json::Value true_;
   Json::Value false_;

   ValueTest()
      : emptyArray_( Json::arrayValue )
      , emptyObject_( Json::objectValue )
      , integer_( 123456789 )
      , smallUnsignedInteger_( Json::Value::UInt( Json::Value::maxInt ) )
      , unsignedInteger_( 34567890u )
      , real_( 1234.56789 )
      , emptyString_( "" )
      , string1_( "a" )
      , string_( "sometext with space" )
      , true_( true )
      , false_( false )
   {
      array1_.append( 1234 );
      object1_["id"] = 1234;
   }

   struct IsCheck
   {
      /// Initialize all checks to \c false by default.
      IsCheck();

      bool isObject_;
      bool isArray_;
      bool isBool_;
      bool isDouble_;
      bool isInt_;
      bool isUInt_;
      bool isIntegral_;
      bool isNumeric_;
      bool isString_;
      bool isNull_;
   };

   void checkConstMemberCount( const Json::Value &value, unsigned int expectedCount );

   void checkMemberCount( Json::Value &value, unsigned int expectedCount );

   void checkIs( const Json::Value &value, const IsCheck &check );
};


JSONTEST_FIXTURE( ValueTest, size )
{
   JSONTEST_ASSERT_PRED( checkMemberCount(emptyArray_, 0) );
   JSONTEST_ASSERT_PRED( checkMemberCount(emptyObject_, 0) );
   JSONTEST_ASSERT_PRED( checkMemberCount(array1_, 1) );
   JSONTEST_ASSERT_PRED( checkMemberCount(object1_, 1) );
   JSONTEST_ASSERT_PRED( checkMemberCount(null_, 0) );
   JSONTEST_ASSERT_PRED( checkMemberCount(integer_, 0) );
   JSONTEST_ASSERT_PRED( checkMemberCount(real_, 0) );
   JSONTEST_ASSERT_PRED( checkMemberCount(emptyString_, 0) );
   JSONTEST_ASSERT_PRED( checkMemberCount(string_, 0) );
   JSONTEST_ASSERT_PRED( checkMemberCount(true_, 0) );
}


JSONTEST_FIXTURE( ValueTest, isObject )
{
   IsCheck checks;
   checks.isObject_ = true;
   JSONTEST_ASSERT_PRED( checkIs( emptyObject_, checks ) );
   JSONTEST_ASSERT_PRED( checkIs( object1_, checks ) );
}


JSONTEST_FIXTURE( ValueTest, isArray )
{
   IsCheck checks;
   checks.isArray_ = true;
   JSONTEST_ASSERT_PRED( checkIs( emptyArray_, checks ) );
   JSONTEST_ASSERT_PRED( checkIs( array1_, checks ) );
}


JSONTEST_FIXTURE( ValueTest, isNull )
{
   IsCheck checks;
   checks.isNull_ = true;
   checks.isObject_ = true;
   checks.isArray_ = true;
   JSONTEST_ASSERT_PRED( checkIs( null_, checks ) );
}


JSONTEST_FIXTURE( ValueTest, isString )
{
   IsCheck checks;
   checks.isString_ = true;
   JSONTEST_ASSERT_PRED( checkIs( emptyString_, checks ) );
   JSONTEST_ASSERT_PRED( checkIs( string_, checks ) );
   JSONTEST_ASSERT_PRED( checkIs( string1_, checks ) );
}


JSONTEST_FIXTURE( ValueTest, isBool )
{
   IsCheck checks;
   checks.isBool_ = true;
   checks.isIntegral_ = true;
   checks.isNumeric_ = true;
   JSONTEST_ASSERT_PRED( checkIs( false_, checks ) );
   JSONTEST_ASSERT_PRED( checkIs( true_, checks ) );
}


JSONTEST_FIXTURE( ValueTest, isDouble )
{
   IsCheck checks;
   checks.isDouble_ = true;
   checks.isNumeric_ = true;
   JSONTEST_ASSERT_PRED( checkIs( real_, checks ) );
}


JSONTEST_FIXTURE( ValueTest, isInt )
{
   IsCheck checks;
   checks.isInt_ = true;
   checks.isNumeric_ = true;
   checks.isIntegral_ = true;
   JSONTEST_ASSERT_PRED( checkIs( integer_, checks ) );
}


JSONTEST_FIXTURE( ValueTest, isUInt )
{
   IsCheck checks;
   checks.isUInt_ = true;
   checks.isNumeric_ = true;
   checks.isIntegral_ = true;
   JSONTEST_ASSERT_PRED( checkIs( unsignedInteger_, checks ) );
   JSONTEST_ASSERT_PRED( checkIs( smallUnsignedInteger_, checks ) );
}


void
ValueTest::checkConstMemberCount( const Json::Value &value, unsigned int expectedCount )
{
   unsigned int count = 0;
   Json::Value::const_iterator itEnd = value.end();
   for ( Json::Value::const_iterator it = value.begin(); it != itEnd; ++it )
   {
      ++count;
   }
   JSONTEST_ASSERT_EQUAL( expectedCount, count ) << "Json::Value::const_iterator";
}

void
ValueTest::checkMemberCount( Json::Value &value, unsigned int expectedCount )
{
   JSONTEST_ASSERT_EQUAL( expectedCount, value.size() );

   unsigned int count = 0;
   Json::Value::iterator itEnd = value.end();
   for ( Json::Value::iterator it = value.begin(); it != itEnd; ++it )
   {
      ++count;
   }
   JSONTEST_ASSERT_EQUAL( expectedCount, count ) << "Json::Value::iterator";

   JSONTEST_ASSERT_PRED( checkConstMemberCount(value, expectedCount) );
}


ValueTest::IsCheck::IsCheck()
   : isObject_( false )
   , isArray_( false )
   , isBool_( false )
   , isDouble_( false )
   , isInt_( false )
   , isUInt_( false )
   , isIntegral_( false )
   , isNumeric_( false )
   , isString_( false )
   , isNull_( false )
{
}


void 
ValueTest::checkIs( const Json::Value &value, const IsCheck &check )
{
   JSONTEST_ASSERT_EQUAL( check.isObject_, value.isObject() );
   JSONTEST_ASSERT_EQUAL( check.isArray_, value.isArray() );
   JSONTEST_ASSERT_EQUAL( check.isBool_, value.isBool() );
   JSONTEST_ASSERT_EQUAL( check.isDouble_, value.isDouble() );
   JSONTEST_ASSERT_EQUAL( check.isInt_, value.isInt() );
   JSONTEST_ASSERT_EQUAL( check.isUInt_, value.isUInt() );
   JSONTEST_ASSERT_EQUAL( check.isIntegral_, value.isIntegral() );
   JSONTEST_ASSERT_EQUAL( check.isNumeric_, value.isNumeric() );
   JSONTEST_ASSERT_EQUAL( check.isString_, value.isString() );
   JSONTEST_ASSERT_EQUAL( check.isNull_, value.isNull() );
}



int main( int argc, const char *argv[] )
{
   JsonTest::Runner runner;
   JSONTEST_REGISTER_FIXTURE( runner, ValueTest, size );
   JSONTEST_REGISTER_FIXTURE( runner, ValueTest, isObject );
   JSONTEST_REGISTER_FIXTURE( runner, ValueTest, isArray );
   JSONTEST_REGISTER_FIXTURE( runner, ValueTest, isBool );
   JSONTEST_REGISTER_FIXTURE( runner, ValueTest, isInt );
   JSONTEST_REGISTER_FIXTURE( runner, ValueTest, isUInt );
   JSONTEST_REGISTER_FIXTURE( runner, ValueTest, isDouble );
   JSONTEST_REGISTER_FIXTURE( runner, ValueTest, isString );
   JSONTEST_REGISTER_FIXTURE( runner, ValueTest, isNull );
   return runner.runCommandLine( argc, argv );
}
