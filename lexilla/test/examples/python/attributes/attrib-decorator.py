# issue#294 also pointed out that decorator attributes behaved differently
#   for left-justified decorators vs indented decorators

@decorator.attribute
def foo():
    @decorator.attribute
    def bar():
        pass
    bar()
