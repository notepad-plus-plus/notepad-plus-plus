# Contributing

The Notepad++ project welcomes your contributions to the project. However, not all contributions will or can be accepted. Only after careful review and deliberation, will a contribution be accepted.

## Guidelines for Pull Requests
Although not every contribution can be accepted and merged into the Notepad++ project, Following these guidelines can help get your contribution accepted.

- Respect the Notepad++ coding style.
- Make a single change per commit.
- Make your modification compact - don't reformat source code in your request. It makes code review more difficult.
- Pull requests consisting of reformatting (Changes made to white space, TAB, line endings, or coding style solely for the purpose of style or to increase/decrease readability) of source code will not be accepted. Use the issue tracker for your request instead.
- Only bug-fix and feature request will be accepted. For everything else, I can run static code analysis tool myself.

**In short:** The *easier* the code review is, the *better* the chance your pull request or other contribution will be accepted.

## Coding style

### General

#### Avoid using pointers.
Avoid using pointers, and prefer references instead. You might need the variable to be assigned a NULL value: in this case the NULL value has semantics and must be checked. Wherever possible, use a SmartPtr instead of old-school pointers.

#### Prefer using enumeration for integer constants.

#### Avoid using new if you can use automatic variable.

#### Don't place any ```using namespace``` directives in headers.

#### Compile time is without incidence. Increasing compile time to reduce execution time is encouraged.

#### Code legibility and length is less important than easy and fast end-user experience.

#### Prefer to prefix increment and decrement operators:
Increment and decrement operators should be prefixed, rather than postfixed.
```c
      ++i
```
**over**
```c
      i++
```
It does not change anything for built-in types, but it should bring consistency
   
#### Avoid the use of magic numbers.

**Bad Example:**
```c
  while (lifeTheUniverseAndEverything != 42)
      lifeTheUniverseAndEverything =  buildMorePowerfulComputerForTheAnswer();
```
**Good Example:**
```c
  if (foo < iCanPushOnTheBigRedButton)
      startThermoNuclearWar();
```

### Indention

Code in the Notepad++ project uses the Allman indention style.

#### Do not use Java-like braces:

**Good Example:**
```c
  if ()
  {
      // Do something
  }
```
**Bad Example:**
  ```c
  if () {
      // Do something
  }
```
#### Use tabs instead of whitespaces for indentation.
The collaborators of the project usually set our editors to 4 whitespaces for 1 tab, but the choice is up to you.

#### Use the following indention style for "switch" control statements.
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

### Spacing

#### Always insert one space before and after any binary, logical, or ternary operators. 

**Good spacing:**
```c
  if (10 == a && 42 == b)
```
**Bad spacing**
  ```c
  if (a==10&&b==42)
```
#### Always insert one space after each semi-colon in "for" statements.

**Good spacing:**
  ```c
  for (int i = 0; i != 10; ++i)
  ```
**Bad spacing:**
 ```c
  for(int i=0;i<10;++i)
```

#### Function names are not to be separated from the parameter list by any whitespace:

**Good spacing**:
```c
    foo();
    myObject.foo(24);
```
**Bad Spacing**
```c
    foo ();
	myObject.foo (24);
```
#### Keywords are separated from the first parenthesis by one space:
Keywords are *not* function calls.
**Good spacing:**
```c
    if (myCondition)
```
**Bad spacing**
```c
    if(myCondition)
```

### Naming Conventions

#### Class names should be in PascalCase.
Every letter in the name is capitalized, including the first letter. No spaces or other separators (such as dashes or underscores) should be used:

**Good naming:**
```c
  class IAmAClass
  {};
```
**Bad naming:**
```c
  class iAmClass
  {};
 ```
 **OR**
 ```c
  class I_am_class
  {};
```

#### Methods or Functions should be in camelCase.
Every word, expect for the first word, should be capitalized. No spaces or other separators (such as dashes or underscores) should be used:

**Good naming**:
```c
  void myMethod();
```

#### Method or Function parameters should be in camelCase.
Every word, expect for the first word, should be capitalized. No spaces or other separators (such as dashes or underscores) should be used:

**Good naming:**:
```c
 void myMethod(uint myVeryLongParameter);
 ```
 
#### Any member variable name of class/struct should be preceded by an underscore and in camelCase.
**Good naming**:
```c
  public:
      int _publicAttribute;
  private:
      int _pPrivateAttribute;
      float _pAccount;
```
	  
#### Use descriptive names for variables.
Always use a variable name that describes what the variable is used for. 

**Good variable naming:**
```c
  if (hours < 24 && minutes < 60 && seconds < 60)
```
**Bad variable naming:**
```c
  if (a < 24 && b < 60 && c < 60)
```

###Comments

#### Use single line comments only.
Use single line comments only in the project's source code. Use single line comments even if your comment spans for more than one line.

**Good comment style:**
```c
  // Two line comment
  // Use still C++ comment line style
```
**Bad comment style:**
```c
  /*
  Please don't piss me off with that
  */
```
**OR**
```c
 /* or with this. */
```
