# Contributing

***Ask not what Notepad++ can do for you - ask what you can do for Notepad++***

## Reporting Issues

Bug reports are appreciated. Following a few guidelines listed below will help speed up the process of getting them fixed.

1. Search the issue tracker to see if it has already been reported.
2. Disable your plugins to see if one of them is the problem. You can do this by renaming your `plugins` folder to something else.
3. Only report an issue with a plugin if it is one of the standard plugins included in the Notepad++ installation. Any other plugin issue should be reported to its respective issue tracker (see e.g. [plugin_list_x86.md](https://github.com/notepad-plus-plus/nppPluginList/blob/master/doc/plugin_list_x86.md) or [plugin_list_x64.md](https://github.com/notepad-plus-plus/nppPluginList/blob/master/doc/plugin_list_x64.md) to find the homepage with further information on that for a plugins). The standard plugins include (for v7.9.5):
    * NppExport
    * Converter
    * mimeTools
4. Fill the complete information: a template will be shown when you create an issue. Please fill the complete information in the template. To fill the field **Debug Information** you can get it from your Notepad++ via menu `?>Debug Info...`. Please take your time to fill these information. If you don't bother to complete the information we need to help you, we won't bother to solve your problem either.
    

## Pull Requests
*The first rule of Notepad++ is: you do not ask for permission to contribute.*<br/>
*The second rule of Notepad++ is: you DO NOT ask for permission to contribute.*

Just do your contribution if you have something to offer, and your pull requests are welcome.<br/>
However, they may not be accepted for various reasons. If you want to make some GUI enhancement like renaming some graphic items or fixing typos, please create the issue instead of the pull requests. All Pull Requests, except for translations and user documentation, need to be attached to a issue on GitHub. For Pull Requests regarding enhancements and questions, the issue must first be approved by one of project's administrators before being merged into the project. An approved issue will have the label `Accepted`. For issues that have not been accepted, you may request to be assigned to that issue.

Opening an issue beforehand allows the administrators and the community to discuss bugs and enhancements before work begins, preventing wasted effort.

### Guidelines for pull requests

1. Respect existing Notepad++ coding style. Observe the code near your intended change, and attempt to preserve the style of that code with the changes you make.
2. Create a new branch for each PR. **Make sure your branch name wasn't used before** - you can add date (for example `patch3_20200528`) to ensure its uniqueness.
3. Single feature or bug-fix per PR.
4. Create a PR with a single commit to make the review process easier.
5. For the PR of translation, don't guess or use the next version number. Use the current version number instead.
6. Make your modification compact - don't reformat source code in your request. It makes code review more difficult.
7. PR of reformatting (changing of ws/TAB, line endings or coding style) of source code won't be accepted. Use issue trackers for your request instead.
8. Typo fixing and code refactoring won't be accepted - please create issues with title started with `TYPO` to request the changing.
9. The PR for the enhancement of Function List parser should also include unit test. Please refer [here](https://npp-user-manual.org/docs/function-list/#contribute-your-new-or-enhanced-parser-rule-to-the-notepad-codebase) for more information. 
10. Address the review change requests by pushing new commits to the same PR. Avoid amending a commit and then force pushing it. All the PR commits are squashed before merging to the main branch.
11. When creating new PR, try to base it on latest master.
12. Normally you don't need to merge `upstream/master` (using git or via github sync), if your PR is based on older `upstream/master`. If you need to base it on latest `master` (e.g. to check and fix merge conflict), use commands `git fetch upstream` to get latest `master` and then `git rebase upstream/master` to rebase it onto this latest `upstream/master`.
13. Finally, please test your pull requests, at least once.

In short: The easier the code review is, the better the chance your pull request will get accepted.

### Coding style

![stay clean](https://notepad-plus-plus.org/assets/images/good-bad-practice.jpg)

#### GENERAL

1. Do not use Java-like braces

    * Good:

    ```cpp
    void MyClass::method1()
    {
        if (aCondition)
        {
            // Do something
        }
    }
    ```

    * Bad:

    ```cpp
    void MyClass::method1() {
        if (aCondition) {
            // Do something
        }
    }
    ```

    However, the method definition could be defined in a header file (.h), if there's one line code only. In this case, Java-like braces should be used.

    * Good:
  
    ```cpp
    class MyClass
    {
    public:
        void method1();
        int method2() {
            return _x; // only one line code can be placed in .h as method definition
        };

    private:
        int _x;
    }
    ```

2. Use tabs instead of white-spaces (we usually set our editors to 4 white-spaces for 1 tab, but the choice is up to you)

3. Always leave one space before and after binary and ternary operators

    * Good:

    ```cpp
    if (a == 10 && b == 42)
    ```

    * Bad:

    ```cpp
    if (a==10&&b==42)
    ```

4. Only leave one space after semi-colons in "for" statements

    * Good:

    ```cpp
    for (int i = 0; i != 10; ++i)
    ```

    * Bad:

    ```cpp
    for(int i=0;i<10;++i)
    ```

5. Function names are not separated from the first parenthesis

    * Good:

    ```cpp
    foo();
    myObject.foo(24);
    ```

    * Bad:

    ```cpp
    foo ();
    ```

6. Keywords are separated from the first parenthesis by one space

    * Good:

    ```cpp
    if (true)
    while (true)
    ```

    * Bad:

    ```cpp
    if(myCondition)
    ```

7. Switch

    * Use the following indenting for "switch" statements:

    ```cpp
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

    * If possible use `default` statement, and prefer using it as last case.
    * When using switch with enum or known range, try to cover all values if not using `default`.

    ```cpp
    enum class Test {val1, val2, val3}

    switch (Test)
    {
        case Test::val1:
        {
            // Do something
            break;
        }

        //case Test::val2:
        //case Test::val3:
        default: 
            // Do something else
    } // No semi-colon here
    ```

    When using `default` adding uncovered values as comments can help to convey intention.

    * Use `[[fallthrough]]` if fall through is intended.

    ```cpp
    switch (test)
    {
        case 1:
        {
            // Do something
        }
        // I want fall through // adding comment can help to convey intention
        [[fallthrough]];

        case 2:
        {
            // Do something
            break;
        }

        default:
            // Do something else
    } // No semi-colon here
    ```

8. Avoid magic numbers

    * Good:

    ```cpp
    if (foo == I_CAN_PUSH_ON_THE_RED_BUTTON)
        startTheNuclearWar();
    ```

    * Bad:

    ```cpp
    while (lifeTheUniverseAndEverything != 42)
        lifeTheUniverseAndEverything = buildMorePowerfulComputerForTheAnswer();
    ```

9. Prefer enums for integer constants

10. Use initialization with curly braces

    * Good:

    ```cpp
    MyClass instance{10.4};
    ```

    * Bad:

    ```cpp
    MyClass instance(10.4);
    ```

11. Always use `empty()` for testing if a string is empty or not

    * Good:

    ```cpp
    if (!string.empty())
    ...
    ```

    * Bad:

    ```cpp
    if (string != "")
    ...
    ```

12. Always use `C++ conversion` instead of `C-Style cast`

    * Generally, all the conversion among types should be avoided. If you have no choice, use C++ conversion.

    * Good:

    ```cpp
    char aChar = static_cast<char>(_pEditView->execute(SCI_GETCHARAT, j));
    ```

    * Bad:

    ```cpp
    char aChar = (char)_pEditView->execute(SCI_GETCHARAT, j);
    ```

13. Use `!` instead of `not`, `&&` instead of `and`, `||` instead of `or`

    * Good:

    ```cpp
    if (!::PathFileExists(dir2Search))
    ```

    * Bad:

    ```cpp
    if (not ::PathFileExists(dir2Search))
    ```

14. Always initialize local and global variables

    * For primitive types and enum prefer initialization with `=`.
    * For other prefer `{}`-initializer syntax.
    * For "numerical" variables using literal suffix can help to convey intention.

    ```cpp
    constexpr float g_globalVariable = 0.0F;

    void test()
    {
      constexpr UINT strLen = 1024U;
      wchar_t myString[strLen]{};
    }
    ```

#### NAMING CONVENTIONS

1. Classes uses Pascal Case

    * Good:

    ```cpp
    class IAmAClass
    {};
    ```

    * Bad:
  
    ```cpp
    class iAmAClass
    {};
    class I_am_a_class
    {};
    ```

2. Methods & method parameters

    * Use camel Case

    ```cpp
    void myMethod(uint myVeryLongParameter);
    ```

3. Member variables

    * Any member variable name of class/struct should be preceded by an underscore.

    ```cpp
    public:
        int _publicAttribute;
    private:
        int _pPrivateAttribute;
        float _pAccount;
    ```

4. Always prefer a variable name that describes what the variable is used for

    * Good:

    ```cpp
    if (hours < 24 && minutes < 60 && seconds < 60)
    ```

   * Bad:

    ```cpp
    if (a < 24 && b < 60 && c < 60)
    ```

#### COMMENTS

1. Use C++ comment line style rather than C comment style

   * Good:

    ```cpp
    // Two lines comment
    // Use still C++ comment line style
    ```

   * Bad:

    ```cpp
    /*
    Please don't piss me off with that
    */
    ```

#### BEST PRACTICES

1. Use C++11/14/17/20 whenever it is possible.

2. Use C++11 member initialization feature whenever it is possible.

    ```cpp
    class Foo
    {
        int value = 0;
    };
    ```

3. Incrementing
    * Prefer Pre-increment

    ```cpp
    ++i
    ```

    * Over Post-increment

    ```cpp
    i++
    ```

   (It does not change anything for built-in types but it would bring consistency)

4. Avoid using pointers. References are preferred instead. You might need the variable to be assigned a `NULL` value: in this case the `NULL` value has semantics and must be checked. Wherever possible, use a SmartPtr instead of old-school pointers.

5. Avoid using new if you can use automatic variable. However, avoid `shared_ptr` as much as possible. Prefer `unique_ptr` instead.

6. Don't place any "using namespace" directives in headers.

7. Compile time is without incidence. Increasing compile time to reduce execution time is encouraged.

8. Code legibility and length is less important than easy and fast end-user experience.

9. Prefer `constexpr` over `const` if value can be evaluated at compile time.

10. Check if there are helper functions in headers or lambda functions to reuse them instead of writing new code.

    * Example

    ```cpp
    // in StaticDialog.h
    isCheckedOrNot();
    setChecked();

    // in Parameters.cpp
    parseYesNoBoolAttribute();
    ```

11. Check if there are already defined global variables, and reuse them instead of defining new ones.

12. Avoid "Yoda conditions".

    * Good:

    ```cpp
    if (iAmYourFather == true)
    ...
    ```

    * Bad:

    ```cpp
    if (true == iAmYourFather)
    ...
    ```

13. Check [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md) for additional guidelines.
