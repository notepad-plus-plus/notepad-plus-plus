// @ used in the non-annotation sense
display "test" @ field.

// [ used in the non-annotation sense
a[b] = 12

// source annotation examples
@sourceannotation
	@source-annotation()
@source_annotation(prop = "abc")
  @source#$%annotation(prop = "abc", prop2 = 123)

// typed annotation examples
[typedannotation]
	[typed-annotation()]
[typed_annotation(prop = "abc")]
  [typed#$%annotation(prop = "abc", prop2 = 123)]
[typedannotation(
    prop = "abc",
    prop2 = 123)]

// we don't care if the property list is malformed; @ or [ followed by a class
// name at the start of a line is always styled as an annotation
@source_annotation(prop = "abc"
[typed_annotation(prop = "abc", prop2 = 123]
@source_annotation

// not an annotation because it's not the first non-whitespace character
bad   @notannotation
  bad   [notannotation]

// poorly formed annotations
@@@ThreeAtSigns // the @s are all operators, no annotation on this line
@name[containsbadcharacter // annotation style stops at bad character
[name[containsbadcharacter // typed annotation style stops at bad character
[name/containsbadcharacter // typed annotation style stops at bad character
[ typedannotation ] // can't have a space between [ and the class name

// array reference on multiple lines
a
[b] = 12 // valid syntax but will be styled as an annotation; lesson: don't do that