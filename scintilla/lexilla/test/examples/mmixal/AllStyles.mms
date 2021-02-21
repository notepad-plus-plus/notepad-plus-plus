% Demonstrate each possible style. Does not make sense as code.

% A comment 1
% Comment


% Whitespace 0
        


% Label 2
label


% Not Validated Opcode 3 appears to always validate to either 5 or 6
% so is never seen on screen.


% Division between Label and Opcode 4
la      


% Valid Opcode 5
        TRAP


% Invalid Opcode 6
        UNKNOWN


% Division between Opcode and Operands 7
        LOC   


% Division of Operands 8
        LOC   0.


% Number 9
        BYTE  0


% Reference 10
        JMP @label


% Char 11
        BYTE 'a'


% String 12
        BYTE "Hello, world!"


% Register 13
        BYTE rA


% Hexadecimal Number 14
        BYTE #FF


% Operator 15
        BYTE  +


% Symbol 16
        TRAP  Fputs


% Preprocessor 17
@include a.mms


