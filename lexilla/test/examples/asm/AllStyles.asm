; Enumerate all styles: 0 to 15 except for 11(comment block) which is not yet implemented.
; This is not a viable source file, it just illustrates the different states in isolation.

; comment=1
; Comment

; whitespace=0
	; w

; number=2
11

; string=3
"String"

; operator=4
+

; identifier=5
identifier

; CPU instruction=6
add

; math Instruction=7
fadd

; register=8
ECX

; directive=9
section

; directive operand=10
rel

; comment block=11 is for future expansion

; character=12
'character'

; string EOL=13
"no line end

; extended instruction=14
movq

; comment directive=15
 comment ~ A multiple-line
 comment directive~

;end
