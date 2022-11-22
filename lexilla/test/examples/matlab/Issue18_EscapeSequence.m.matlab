a="""";
b=1;
c='\';
d=2;
e="\";
f=3;
%" this should be a comment (colored as such), instead it closes the string
g="
h=123;
%" this is a syntax error in Matlab (about 'g'),
% followed by a valid assignment (of 'h')
% Instead, 'h' is colored as part of the string

% Octave terminates string at 3rd ", Matlab at 4th
i="\" "; % " %
% end
