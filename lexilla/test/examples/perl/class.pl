#!/usr/bin/env perl
use v5.38;
use feature 'class';

class MyClass::SubClass {
    method inClass { return 1 }
    method inClassProto($) { return $_[0] }
    method inClassAttrib :prototype($) { return $_[0] }
}
