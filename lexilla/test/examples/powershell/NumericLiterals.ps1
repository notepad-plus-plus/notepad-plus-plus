# Treat any leading [-+] as default to reduce match complexity
# https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_numeric_literals?view=powershell-7.3#examples
100
100u
100D
100l
100uL
100us
100uy
100y
1e2
1.e2
0x1e2
0x1e2L
0x1e2D
482D
482gb
482ngb
0x1e2lgb
0b1011011
0xFFFFs
0xFFFFFFFF
-0xFFFFFFFF
0xFFFFFFFFu

# Float
0.5
.5

# Range
1..100

# Issue118: 7d is numeric while 7z is user defined keyword
7d
7z
