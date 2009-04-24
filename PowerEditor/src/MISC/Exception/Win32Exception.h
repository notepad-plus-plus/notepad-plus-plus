//This code was retrieved from
//http://www.thunderguy.com/semicolon/2002/08/15/visual-c-exception-handling/3/
//(Visual C++ exception handling)
//By Bennett
//Formatting Slightly modified for N++

#include "windows.h"
#include <exception>

typedef const void* ExceptionAddress; // OK on Win32 platform

class Win32Exception : public std::exception
{
public:
    static void 		installHandler();
    static void 		removeHandler();
    virtual const char* what()  const throw() { return _event;    };
    ExceptionAddress 	where() const         { return _location; };
    unsigned int		code()  const         { return _code;     };
	EXCEPTION_POINTERS* info()  const         { return _info;     };
	
protected:
    Win32Exception(EXCEPTION_POINTERS * info);	//Constructor only accessible by exception handler
    static void 		translate(unsigned code, EXCEPTION_POINTERS * info);

private:
    const char * _event;
    ExceptionAddress _location;
    unsigned int _code;

	EXCEPTION_POINTERS * _info;
};

class Win32AccessViolation: public Win32Exception
{
public:
    bool 				isWrite()    const { return _isWrite;    };
    ExceptionAddress	badAddress() const { return _badAddress; };
private:
    Win32AccessViolation(EXCEPTION_POINTERS * info);

    bool _isWrite;
    ExceptionAddress _badAddress;

    friend void Win32Exception::translate(unsigned code, EXCEPTION_POINTERS* info);
};
