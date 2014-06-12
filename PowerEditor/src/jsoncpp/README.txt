* Introduction:
  =============

JSON (JavaScript Object Notation) is a lightweight data-interchange format. 
It can represent integer, real number, string, an ordered sequence of 
value, and a collection of name/value pairs.

JsonCpp is a simple API to manipulate JSON value, handle serialization 
and unserialization to string.

It can also preserve existing comment in unserialization/serialization steps,
making it a convenient format to store user input files.

Unserialization parsing is user friendly and provides precise error reports.


* Building/Testing:
  =================

JsonCpp uses Scons (http://www.scons.org) as a build system. Scons requires
python to be installed (http://www.python.org).

You download scons-local distribution from the following url:
http://sourceforge.net/project/showfiles.php?group_id=30337&package_id=67375

Unzip it in the directory where you found this README file. scons.py Should be 
at the same level as README.

python scons.py platform=PLTFRM [TARGET]
where PLTFRM may be one of:
	suncc Sun C++ (Solaris)
	vacpp Visual Age C++ (AIX)
	mingw 
	msvc6 Microsoft Visual Studio 6 service pack 5-6
	msvc70 Microsoft Visual Studio 2002
	msvc71 Microsoft Visual Studio 2003
	msvc80 Microsoft Visual Studio 2005
	linux-gcc Gnu C++ (linux, also reported to work for Mac OS X)
	
adding platform is fairly simple. You need to change the Sconstruct file 
to do so.
	
and TARGET may be:
	check: build library and run unit tests.

    
* Running the test manually:
  ==========================

cd test
# This will run the Reader/Writer tests
python runjsontests.py "path to jsontest.exe"

# This will run the Reader/Writer tests, using JSONChecker test suite
# (http://www.json.org/JSON_checker/).
# Notes: not all tests pass: JsonCpp is too lenient (for example,
# it allows an integer to start with '0'). The goal is to improve
# strict mode parsing to get all tests to pass.
python runjsontests.py --with-json-checker "path to jsontest.exe"

# This will run the unit tests (mostly Value)
python rununittests.py "path to test_lib_json.exe"

You can run the tests using valgrind:
python rununittests.py --valgrind "path to test_lib_json.exe"


* Building the documentation:
  ===========================

Run the python script doxybuild.py from the top directory:

python doxybuild.py --open --with-dot

See doxybuild.py --help for options. 


* Adding a reader/writer test:
  ============================

To add a test, you need to create two files in test/data:
- a TESTNAME.json file, that contains the input document in JSON format.
- a TESTNAME.expected file, that contains a flatened representation of 
  the input document.
  
TESTNAME.expected file format:
- each line represents a JSON element of the element tree represented 
  by the input document.
- each line has two parts: the path to access the element separated from
  the element value by '='. Array and object values are always empty 
  (e.g. represented by either [] or {}).
- element path: '.' represented the root element, and is used to separate 
  object members. [N] is used to specify the value of an array element
  at index N.
See test_complex_01.json and test_complex_01.expected to better understand
element path.


* Understanding reader/writer test output:
  ========================================

When a test is run, output files are generated aside the input test files. 
Below is a short description of the content of each file:

- test_complex_01.json: input JSON document
- test_complex_01.expected: flattened JSON element tree used to check if 
    parsing was corrected.

- test_complex_01.actual: flattened JSON element tree produced by 
    jsontest.exe from reading test_complex_01.json
- test_complex_01.rewrite: JSON document written by jsontest.exe using the
    Json::Value parsed from test_complex_01.json and serialized using
    Json::StyledWritter.
- test_complex_01.actual-rewrite: flattened JSON element tree produced by 
    jsontest.exe from reading test_complex_01.rewrite.
test_complex_01.process-output: jsontest.exe output, typically useful to
    understand parsing error.
