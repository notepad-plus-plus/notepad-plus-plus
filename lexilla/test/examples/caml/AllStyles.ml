(* Enumerate all styles: 0 to 15 *)
(* comment=12 *)

(* whitespace=0 *)
	(* w *)

(* identifier=1 *)
ident

(* tagname=2 *)
`ident

(* keyword=3 *)
and

(* keyword2=4 *)
None

(* keyword3=5 *)
char

(* linenum=6 *)
#12

(* operator=7 *)
*

(* number=8 *)
12

(* char=9 *)
'a'

(* white=10 *)
(* this state can not be reached in caml mode, only SML mode but that stops other states *)
(* SML mode is triggered by "andalso" being in the keywords *)
"\ \x"

(* string=11 *)
"string"

(* comment1=13 *)
(* (* comment 1 *) *)

(* comment2=14 *)
(* (* (* comment 2 *) *) *)

(* comment3=15 *)
(* (* (* (* comment 1 *) *) *)  *)
