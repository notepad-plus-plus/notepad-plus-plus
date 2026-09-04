// regex.cxx Test Javascript regex literal support
// Has .cxx extension but really JavaScript.

// Basic
x = /\d+/

// Flags following
x = /x_\w+/iu

// Single character
x = /a/

// / inside set
x = /\w+[+-*/]/

// Empty -> Line comment
//

// Line end before terminating / so operator not regex.
x = /a
/
