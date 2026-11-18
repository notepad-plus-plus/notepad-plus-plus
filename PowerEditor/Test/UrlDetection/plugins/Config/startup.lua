-- Startup script for URL detection verification test
endNppAfterUrlTest = 1
local nppDir = npp:GetNppDirectory()
local verifyUrlDetection = loadfile(nppDir .."\\" .. "..\\Test\\UrlDetection\\verifyUrlDetection.lua")
pcall(verifyUrlDetection)

