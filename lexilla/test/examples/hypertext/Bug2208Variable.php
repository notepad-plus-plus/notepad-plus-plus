<?php
$foo = 'bar';
$world = 'foo';
$a = 'hello';
$$a = 'world';

echo $$a.PHP_EOL;
echo $$$a.PHP_EOL;
echo "$a {$$a} $$a {$$$a}\n";
echo "$a ${$a} $$a ${$$a}\n";
?>
