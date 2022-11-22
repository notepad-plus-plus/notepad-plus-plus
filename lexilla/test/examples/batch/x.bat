rem comment=1
rem 'echo' is word=2, 'a' is default=0
echo a
rem label=3
:START
rem '@' is hide=4
@echo b
rem 'gcc' is external command=5
gcc --version
rem '%PATH%' is variable=6
echo %PATH%
echo %ProgramFiles(x86)%
rem operator=7 '='
@set Q=A

::comment=1

:: Bug 1624: this construct produced inconsistent brackets in the past
if ERRORLEVEL 2 goto END
@if exist a (
echo exists
) else (
echo not
)

FOR /L %%G IN (2,1,4) DO (echo %%G)

:: Bug 1997: keywords not recognized when preceded by '('
IF NOT DEFINED var (SET var=1)

:: Bug 2065: keywords not recognized when followed by ')'
@if exist a ( exit)

:: Bug: with \r or \n, 'command' is seen as continuation
echo word ^
1
command

:: Bug argument and variable expansion
echo %~dp0123
echo %%-~012
echo %%~%%~-abcd
FOR /F %%I in ("C:\Test\temp.txt") do echo %%~dI

:: Bug ending of argument and variable expansion
echo %~dp0\123
echo "%~dp0123"
echo "%%-~012"
echo "%%~%%~-abcd"
FOR /F %%I in ("C:\Test\temp.txt") do echo "%%~dI"

:: Bug escaped %
echo %%0
echo %%%0
echo %%%%~-abcd

:: Bug 2304: "::" comments not recognised when second command on line
Set /A xxx=%xxx%+1 & :: Increment
Set /A xxx=%xxx%+1 & ::Increment
Set /A xxx=%xxx%+1 & rem Increment

:END
