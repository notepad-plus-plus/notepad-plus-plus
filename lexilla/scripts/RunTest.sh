# Test lexers
# build lexilla.so and TestLexers then run TestLexers
JOBS="--jobs=$(getconf _NPROCESSORS_ONLN)"
(
cd ../src
make "$JOBS" DEBUG=1
)
(
cd ../test
make DEBUG=1
make test
)
