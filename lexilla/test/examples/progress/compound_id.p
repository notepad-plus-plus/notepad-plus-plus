// all of "Progress.Lang.Object" is styled as SCE_ABL_IDENTIFIER
DEFINE VARIABLE obj1 AS Progress.Lang.Object.

// "Object" by itself is styled as SCE_ABL_WORD when not part of a compound identifier
DEFINE VARIABLE obj1 AS OBJECT.

// "Object2" isn't a keyword so it's styled as SCE_ABL_IDENTIFIER
DEFINE VARIABLE obj1 AS Object2.

// "name" (which is a keyword) is styled as SCE_ABL_IDENTIFIER when part of a compound identifier
DISPLAY customer.name.
DISPLAY sports.customer.name.

// "name" by itself is styled as SCE_ABL_WORD when not part of a compound identifier
DISPLAY name.

// make sure that SCE_ABL_IDENTIFIER spans an identifier which contains a hyphen
DISPLAY customer.cust-num.
DISPLAY customer.cust-name.

// underscore is a valid character at the beginning of an identifier
DISPLAY _user._user-id.

// - is not a valid character at the beginning of an identifier
DISPLAY sports.-address.

// the attribute is styled as SCE_ABL_WORD
MESSAGE customer.name:VISIBLE.

// filename and extension is styled as SCE_ABL_IDENTIFIER
RUN dir1/dir2/myprocedure.p.
RUN dir1/dir2/name.p
RUN myprocedure.p
RUN name.p

// .* in USING statement is styled as SCE_ABL_IDENTIFIER
USING System.Windows.Forms.*.

// * in an identifier (not following . as above) is invalid
DEFINE VARIABLE a AS Progress.Lang*.
