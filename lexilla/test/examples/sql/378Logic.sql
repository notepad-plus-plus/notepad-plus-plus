-- Issue 378 
-- Demonstrate PL/SQL logic folding and multi-line q string literal.
DECLARE
  a NUMBER := 10; 
  b NUMBER := 0;
  c NUMBER;
  t VARCHAR2 := q'[Single line test]';
  v VARCHAR2 := q'[
a
b
]' + q'(1)'; -- Test whether stale value ')' leaks back to line start.
BEGIN 
  c := a / b; 
-- Division by zero  DBMS_OUTPUT.PUT_LINE('Result: ' || c);
EXCEPTION  
  WHEN ZERO_DIVIDE THEN
    DBMS_OUTPUT.PUT_LINE(q'(Error:
Division by zero is not allowed.)');
    DBMS_OUTPUT.PUT_LINE(q'!Not a complemented quote.!');
END;
