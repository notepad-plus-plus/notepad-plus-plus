<%@language=javas%>
<% 
#include 
serve x;
function x() {
}
%>
<%@language=vbscript%>
<% 
sub x 'comment 
peek 1024
%>
<!-- Folding for Python is incorrect. See #235. -->
<%@language=python%>
<% 
import random
x = 'comment'
parse "x=8"
%>
<head>
<body></body>
