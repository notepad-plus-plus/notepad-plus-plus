# Bug #2019 Buffer over-read in MMIXAL lexer
label
        PREFIX  Foo:
% Relative reference (uses PREFIX)
        JMP label
%
        JMP @label
% Absolute reference (does not use PREFIX)
        JMP :label
% In register list so treated as register
        JMP :rA
% Too long for buffer so truncated
        JMP l1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
% Too long for buffer so truncated then treated as absolute
        JMP :l1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
%
