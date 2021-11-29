% Some example code

        % Set the address of the program initially to 0x100.
        LOC   #100

Main    GETA  $255,string

        TRAP  0,Fputs,StdOut

        TRAP  0,Halt,0

string  BYTE  "Hello, world!",#a,0
