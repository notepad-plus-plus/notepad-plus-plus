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