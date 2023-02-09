# Match and case as keywords
match (x):
    case +1:
        pass
    case -1:
        pass
    case []:
        pass
    
# Match and case as identifiers
match = 1
def match():
    pass
match.group()
1 + match
case.attribute

# Unfortunately wrong classifications; should be rare in real code because
# non-call expressions usually don't begin lines, the exceptions are match(x)
# and case(x)
match(x)
case(x)
match + 1
case + 1
case[1]
