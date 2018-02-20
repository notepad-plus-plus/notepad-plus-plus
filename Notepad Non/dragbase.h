#ifndef DRAGBASE_H_
#define DRAGBASE_H_

#include <Windows.h>

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#ifdef BUILD_DRAGBASE
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif


/*
Callback procedure called by dragbase
Parameters:
	filename - file name including path where the file has to be stored
	directory - directory where the file has to be stored
	param - optional param which was specified with the Create method

Return:
	Return true if successful or false (=0) if not
*/

typedef BOOL (__stdcall *SaveFileProc )( const char * const, const char * const, void * );

// Errors which can occure when we create dragbase
enum dragbaseError
{
	DBE_OK = 0,
	DBE_INVALIDPARAMS,
	DBE_CLASSREGISTRATIONFAILED,
	DBE_WINDOWCREATIONFAILED,
	DBE_MOUSEHOOKFAILED,
	DBE_HOOKFAILED
};

/*	Adds the dragbase Icon to your window
	Parameters:
		hwnd - Handle to the window where dragbase is stored
		save_file_proc - Callback procedure called by dragbase to save the file (see above)
		app_name - Short string identifier of your application 
		param - optional parameter passed to the callback function
	Return: dragbaseError
*/
EXTERNC  DLLEXPORT unsigned __stdcall Create(HWND hwnd, SaveFileProc save_file_proc, const char * const app_name,
								void * param);


// Set the file name used for saving the file in a temporary directory
// Parameters:
//	filename - The new file name (withouth any path)
EXTERNC DLLEXPORT void __stdcall SetFilename(const char * const filename);



#endif