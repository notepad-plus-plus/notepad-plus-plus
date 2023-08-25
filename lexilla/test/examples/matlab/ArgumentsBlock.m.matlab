%% Correctly defined arguments block
function y = foo (x)
% Some comment here
% And, maybe, here

arguments
    x (1,2) {mustBeReal(x)}
end

y = x*2;
arguments = 1;
y = y + arguments;
end

%% No arguments block, "arguments" is used 
%  as a variable name (identifier)
% Prevent arguments from folding with an identifier
function y = foo (x)
% Some comment here
x = x + 1;
arguments = 10;
y = x + arguments;
end

% Prevent arguments from folding with a number
function y = foo (x)
4
arguments = 10;
y = x + arguments;
end

% With a double quote string
function y = foo (x)
"test"
arguments = 10;
y = x + arguments;
end

% With a string
function y = foo (x)
'test'
arguments = 10;
y = x + arguments;
end

% With a keyword
function y = foo (x)
if x == 0;
    return 0;
end
arguments = 10;
y = x + arguments;
end

% With an operator (illegal syntax)
function y = foo (x)
*
arguments = 10;
y = x + arguments;
end

% Semicolon is equivalent to a comment
function y = foo(x)
;;;;;;;
arguments
    x
end
y = x + 2;
end

% Arguments block is illegal in nested functions,
% but lexer should process it anyway
function y = foo (x)
arguments
    x (1,2) {mustBeReal(x)}
end

    function y = foo (x)
    arguments
        x (1,2) {mustBeReal(x)}
    end
    var = 0;
    arguments = 5;
    y = arguments + x;
    end

% Use as a variable, just in case
arguments = 10;
end

% Erroneous use of arguments block
function y = foo(x)
% Some comment
x = x + 1;
arguments
    x
end
y = x;
end

% "arguments" is an argument name too
function r = foo(x, arguments)
arguments
    x
    arguments
end
r = bar(x, arguments{:});
end

% Multiple arguments blocks
function [a, b] = foo(x, y, varargin)

arguments(Input)
    x (1,4) {mustBeReal}
    y (1,:) {mustBeInteger} = x(2:end);
end

arguments(Input, Repeating)
    varargin
end

arguments(Output)
    a (1,1) {mustBeReal}
    b (1,1) {mustBeNonNegative}
end

var = 10;
arguments = {"now", "it's", "variable"};

[a, b] = bar(x, y, arguments);

end

% One line function with arguments block.
% This code style is rarely used (if at all), but the
% lexer shouldn't break
function y = foo(x); arguments; x; end; y = bar(x); end
