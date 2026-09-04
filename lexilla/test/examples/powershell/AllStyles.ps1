# Enumerate all styles: 0 to 16

# line comment = 1
# more comment

# whitespace = 0
    # spaces

# string = 2
"a string"

# character = 3
'c'

# number = 4
123

# variable = 5
$variable

# operator = 6
();

# identifier = 7
identifier

# keyword = 8
break
;

# cmdlet = 9
Write-Output "test output"

# alias = 10
chdir C:\Temp\

# function = 11
Get-Verb -Group Security

# user-defined keyword = 12
lexilla

# multi-line comment = 13
<#
multi-line comment
#>

# here string = 14
@"
here string double
"@

# here string single quote = 15
@'
here string single
'@

# comment keyword = 16
<#
.synopsis
End of file.
#>
