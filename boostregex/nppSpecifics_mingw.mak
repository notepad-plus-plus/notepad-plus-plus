# This makefile should be listed after main Scintilla makefile
# by using the -f option for both makefiles

# Set your boost path (the root of where you've unpacked your boost zip).
# Boost is available from www.boost.org


DIR_BOOST = ../../boostregex

vpath %.h $(DIR_BOOST)
vpath %.cxx $(DIR_BOOST)

INCLUDES += -I $(DIR_BOOST)
CXXFLAGS += -DSCI_OWNREGEX -DBOOST_REGEX_STANDALONE

BOOST_OBJS = \
	$(DIR_O)/BoostRegExSearch.o \
	$(DIR_O)/UTF8DocumentIterator.o

$(BOOST_OBJS): $(DIR_O)/%.o: %.cxx
	$(CXX) $(CXX_ALL_FLAGS) $(CXXFLAGS) -MMD -c $< -o $@

$(LIBSCI): $(BOOST_OBJS)

-include $(BOOST_OBJS:.o=.d)

