rem Test lexers
rem build lexilla.dll and TestLexers.exe then run TestLexers.exe
cd ../src
make --jobs=%NUMBER_OF_PROCESSORS% DEBUG=1
cd ../test
make DEBUG=1
make test
