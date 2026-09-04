-- Enumerate all styles: 0 to 22, except for 3 (variable) and 11 (string) which are not implemented
-- Hidden commands with other states (ored with ox40) are also possible inside /*! ... */

/* commentline=2 */
# comment

# default=0
	# w

# comment=1
/* comment */

# variable=3 is not implemented
@variable

# systemvariable=4
@@systemvariable

# knownsystemvariable=5
@@known

# number=6
6

# majorkeyword=7
select

# keyword=8
in

# databaseobject=9
object

# procedurekeyword=10
procedure

# string=11 is not implemented

# sqstring=12
'string'

# dqstring=13
"string"

# operator=14
+

# function=15
function()

# identifier=16
identifier

# quotedidentifier=17
`quoted`

# user1=18
user1

# user2=19
user2

# user3=20
user3

# hiddencommand=21
/*!*/

# placeholder=22
<{placeholder>}
