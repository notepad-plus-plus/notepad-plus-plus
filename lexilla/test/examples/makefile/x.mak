# '# comment' comment=1
# comment

# '.SUFFIXES' target=5, ':' operator=4
.SUFFIXES:

# 'LD' identifier=3, '=' operator=4, 'link' default=0
LD=link

# '!IFDEF DEBUG' NMAKE preprocessor=2
!IFDEF DEBUG

# 'ifdef DEBUG' GNI make directive default=0
ifdef DEBUG

# '$(' ID EOL=9
X=$(

# Recipe with variable reference $(CXX)
cake.o: cake.cxx
	$(CXX) -c $< -o $@

# End of file
