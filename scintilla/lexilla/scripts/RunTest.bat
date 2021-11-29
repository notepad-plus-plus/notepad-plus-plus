rem Test lexers
rem build lexilla.dll and TestLexers.exe then run TestLexers.exe
cd ../src
make
cd ../test
make
make test
