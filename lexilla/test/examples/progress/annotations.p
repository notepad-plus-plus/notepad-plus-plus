// @ used in the non-annotation sense
display "test" @ field.

// source annotation examples
@sourceannotation
@source-annotation()
@source_annotation(prop = "abc")
@source#$%annotation(prop = "abc", prop2 = 123)

// typed source annotation examples
@@sourceannotation
@@source-annotation()
@@source_annotation(prop = "abc")
@@source#$%annotation(prop = "abc", prop2 = 123)

// poorly formed annotations
@@@ThreeAtSigns // first two @ signs are typed, next is not typed
@name[containsbadcharacter // annotation style stops at bad character
@@name[containsbadcharacter // typed annotation style stops at bad character
