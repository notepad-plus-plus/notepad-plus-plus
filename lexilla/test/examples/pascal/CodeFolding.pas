// tests for code folding

// multi line comments
{ 
line1
line2
}

// begin .. end
begin
some commands
end;

record test
  var1: type1;
  var2: type2;
  end; //record
  
//asm
asm
  some statement
  end; //asm
  
//try (from https://wiki.freepascal.org/Try)
try
  // code that might generate an exception
except
  // will only be executed in case of an exception
  on E: EDatabaseError do
    ShowMessage( 'Database error: '+ E.ClassName + #13#10 + E.Message );
  on E: Exception do
    ShowMessage( 'Error: '+ E.ClassName + #13#10 + E.Message );
end;
  
//try nested (from https://wiki.freepascal.org/Try)
try
  try
    // code dealing with database that might generate an exception
  except
    // will only be executed in case of an exception
    on E: EDatabaseError do
      ShowMessage( 'Database error: '+ E.ClassName + #13#10 + E.Message );
    on E: Exception do
      ShowMessage( 'Error: '+ E.ClassName + #13#10 + E.Message );
  end;
finally
  // clean up database-related resources
end;

//case
case x of
  1: do something;
  2: do some other thing;
else
  do default;
  end; //case
  
//if then else  
if x=y then 
  do something;
else
  do some other thing;
  
//for loop  
for i:=1 to 10 do
  writeln(i)

//do until
repeat
  write(a);
  i:=i+1;
until i>10;

//preprocessor if, else, endif
{$DEFINE label}
{$IFDEF label}
  command 1
{$ELSE}
  command 2
{$ENDIF}
