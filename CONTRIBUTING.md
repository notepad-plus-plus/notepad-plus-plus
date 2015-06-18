#Contributing

Your pull requests are welcome; however, they may not be accepted for various reasons.

##Guidelines for pull requests:

1. Respect Notepad++ coding style.
2. Make a single change per commit.
3. Make your modification compact - don't reformat source code in your request. It makes code review more difficult.
4. PR of reformatting (changing of ws/TAB, line endings or coding style) of source code won't be accepted. Use issue trackers for your request instead.
5. Only bug-fix and feature request will be accepted. For the other things, I can run static code analysis tool myself.

In short: The easier the code review is, the better the chance your pull request will get accepted.


##Coding style:

####GENERAL

* Do not use Java-like braces:

GOOD:
```c
  if ()
  {
      // Do something
  }
```
BAD:
  ```c
  if () {
      // Do something
  }
```
* Use tabs instead of whitespaces (we usually set our editors to 4
  whitespaces for 1 tab, but the choice is up to you)


* Always leave one space before and after binary and ternary operators
  Only leave one space after semi-colons in "for" statements.

GOOD:
```c
  if (10 == a && 42 == b)
```
BAD:
  ```c
  if (a==10&&b==42)
```
GOOD:
  ```c
  for (int i = 0; i != 10; ++i)
  ```
BAD:
  ```c
  for(int i=0;i<10;++i)
```
* Keywords are not function calls.
  Function names are not separated from the first parenthesis:

GOOD:
```c
    foo();
    myObject.foo(24);
```
BAD:
```c
    foo ();
```
* Keywords are separated from the first parenthesis by one space :

GOOD:
```c
    if (true)
    while (true)
```
BAD:
```c
    if(myCondition)
```

* Use the following indenting for "switch" statements
```c
  switch (test)
  {
      case 1:
	  {
	      // Do something
	      break;
	  }
	  default:
	      // Do something else
  } // No semi-colon here
```

* Avoid magic numbers

BAD:
```c
  while (lifeTheUniverseAndEverything != 42)
      lifeTheUniverseAndEverything =  buildMorePowerfulComputerForTheAnswer();
```
GOOD:
```c
  if (foo < I_CAN_PUSH_ON_THE_RED_BUTTON)
      startThermoNuclearWar();
```

* Prefer enums for integer constants



####NAMING CONVENTIONS

* Classes (camel case) :

GOOD:
```c
  class IAmAClass
  {};
```
BAD:
```c
  class iAmClass
  {};
  class I_am_class
  {};
```

* methods (camel case + begins with a lower case)
  method parameters (camel case + begins with a lower case)

GOOD:
```c
  void myMethod(uint myVeryLongParameter);
```
* member variables
  Any member variable name of class/struct should be preceded by an underscore
```c
  public:
      int _publicAttribute;
  private:
      int _pPrivateAttribute;
      float _pAccount;
```
	  
* Always prefer a variable name that describes what the variable is used for

GOOD:
```c
  if (hours < 24 && minutes < 60 && seconds < 60)
```
BAD:
```c
  if (a < 24 && b < 60 && c < 60)
```

####COMMENTS

* Use C++ comment line style than c comment style

GOOD:
```
  // Two lines comment
  // Use still C++ comment line style
```
BAD:
```
  /*
  Please don't piss me off with that
  */
```


####BEST PRACTICES

* Prefer this form :
```c
      ++i
```
   to
```c
      i++
```
   (It does not change anything for builtin types but it would bring consistency)


* Avoid using pointers. Prefer references. You might need the variable to
  be assigned a NULL value: in this case the NULL value has semantics and must
  be checked. Wherever possible, use a SmartPtr instead of old-school pointers.

* Avoid using new if you can use automatic variable.

* Don't place any "using namespace" directives in headers

* Compile time is without incidence. Increasing compile time to reduce execution
  time is encouraged.

* Code legibility and length is less important than easy and fast end-user experience.
