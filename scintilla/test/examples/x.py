# Convert all punctuation characters except '_', '*', and '.' into spaces.
def depunctuate(s):
	'''A docstring'''
	"""Docstring 2"""
	d = ""
	for ch in s:
		if ch in 'abcde':
			d = d + ch
		else:
			d = d + " "
	return d
