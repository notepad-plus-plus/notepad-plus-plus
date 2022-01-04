# Test lexers
# build lexilla.so and TestLexers then run TestLexers
cd ../src
make DEBUG=1
cd ../test
make DEBUG=1
make test
