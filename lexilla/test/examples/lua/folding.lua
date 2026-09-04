--[[ coding:UTF-8
folding structure examples ]]

-- Use all the folding keywords:
--    do end function if repeat until while
function first()
   -- Comment
   if op == "+" then
      r = a + b
    elseif op == "-" then
      r = a - b
    elseif op == "*" then
      r = a*b
    elseif op == "/" then
      r = a/b
    else
      error("invalid operation")
    end

    for i=1,10 do
      print(i)
    end

    while a[i] do
      print(a[i])
      i = i + 1
    end

    -- print the first non-empty line
    repeat
      line = io.read()
    until line ~= ""
    print(line)

end

-- { ... } folds
markers = {
     256,
     128,
}
