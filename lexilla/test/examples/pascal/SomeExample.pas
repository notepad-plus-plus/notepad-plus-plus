// some example source code

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

{ --------------------------------------------------------------------------- }
function functionname(paramerter1: type1):result1;
  var
    i: LongInt;
  begin
  for i:=1 to 10 do
    begin
    writeln(i)
    end;
  result:=true;
  end;
{ --------------------------------------------------------------------------- }
procedure procedurename(parameter2: type2);
  var
    i: LongInt;
  begin
  for i:=1 to 10 do
    begin
    writeln(i)
    end;
  end;
