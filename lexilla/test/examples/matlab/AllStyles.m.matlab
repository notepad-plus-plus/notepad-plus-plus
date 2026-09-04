% Examples of each style 0..8 except for SCE_MATLAB_COMMAND(2) which has a line ending bug

% White space=0
   %

% Comment=1
% Line comment

% Next line is comment in Ocatve but not Matlab
# Octave comment

%{
Block comment.
%}

% Command=2

%{
Omitted as this places a style transiton between \r and \n
!rmdir oldtests
%}

% Number=3
33.3

% Keyword=4
global x

% Single Quoted String=5
'string'

% Operator=6
[X,Y] = meshgrid(-10:0.25:10,-10:0.25:10);

% Identifier=7
identifier = 2

% Double Quoted String=8
"string"

% This loop should fold
for i = 1:5
    x(i) = 3 * i;
end
