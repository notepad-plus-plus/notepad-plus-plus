# Comment

test = 5 # Comment

$(info $(test)) # Comment

clean: # Comment
	echo # Not comment

# Quoting a #
HASH = \#

# Inside variable reference
OUT = $(#SYM)

# Inside function call
X = $(subst /,#,$1)
