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

function t1() {return 1
/2/3;}
function t2() {return Math.PI
/2/3;}
function t3() {return
/a/;}

// Line end before terminating / so operator not regex.
x = /a
/
