<?php

$number = 1_234;
$var = 'variable value';
$test = [
	<<<EOTEST
	string $var string EOTEST
	EOTEST_NOT
	EOTEST,
	0b00_01,
	<<<"EOTEST"
	"string" "$var" "string" EOTEST
	EOTEST_NOT
	EOTEST,
	0x00_02,
	<<<'EOTEST'
	'string' '$var' 'string' EOTEST
	EOTEST_NOT
	EOTEST,
	0x00_03,
];
print_r($test);

# Attribute tests
#[SingleLineAnnotation('string', 1, null)]
#[
	MultiLineAnnotation('string', 1, null)
]

?>
