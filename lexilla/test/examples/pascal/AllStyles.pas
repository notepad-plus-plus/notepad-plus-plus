// Enumerate all primary styles: 0 to 14

{
  SCE_PAS_DEFAULT=0
  SCE_PAS_IDENTIFIER=1
  SCE_PAS_COMMENT=2
  SCE_PAS_COMMENT2=3
  SCE_PAS_COMMENTLINE=4
  SCE_PAS_PREPROCESSOR=5
  SCE_PAS_PREPROCESSOR2=6
  SCE_PAS_NUMBER=7
  SCE_PAS_HEXNUMBER=8
  SCE_PAS_WORD=9
  SCE_PAS_STRING=10
  SCE_PAS_STRINGEOL=11
  SCE_PAS_CHARACTER=12
  SCE_PAS_OPERATOR=13
  SCE_PAS_ASM=14
}

// default=0
   
// identifier=1
function functionname(var paramerter1: type1):result1;
procedure procedurename(const parameter2: type2);

// comment=2
{comment text}

// comment2=3
(* comment text *)

// commentline=4 
// example line

// preprocessor=5
{$DEFINE xyz}

{$IFDEF xyz}
   codeblock 1
{$else}
   codeblock 2
{$endif}

// preprocessor2=6
(*$DEFINE xyz*)

// number=7
123
1.23
-123
-12.3
+123
123
1.23e2
-1.23E2

// hexnumber=8
$123
$123ABCDEF
$ABCDEF123

// word=9
absolute abstract and array as 

// string=10
'string'

// stringeol=11
'string

// character=12
#65

// operator=13 
$ & * + / < = > ^  

// asm
asm 
  this is 
  inside assembler
end
