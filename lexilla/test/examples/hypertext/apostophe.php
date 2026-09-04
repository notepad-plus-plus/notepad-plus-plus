<?php
# Test that currently fails as comment style not started in a number.
# line-comment
// line-comment
/* comment */
$foo = 0#comment
$foo = 0//comment
$foo = 0/*'*/;
?>

<br />
