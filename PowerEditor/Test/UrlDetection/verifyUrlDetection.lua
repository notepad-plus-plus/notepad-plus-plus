local testFiles = {"verifyUrlDetection_1a",
                   "verifyUrlDetection_1b"}
local URL_INDIC = 8
local timerInterval = 10

local curPos = 0
local task = -1
local uFrom = 0
local uTo = 0
local mFrom = 0
local mTo = 0
local OKorKO = "OK"
local nFile = 1
local testResults = {}
local outFile = nil

local function Summary()
   local resLine = ""
   local i = 1
   while testFiles[i] ~= nil do
     if  testResults[i] == nil then
       testResults[i] = "KO"
     end
     print(testFiles[i] .. ": " .. testResults[i])
     i = i + 1
   end
   print(resLine)
   if endNppAfterUrlTest ~= nil then
      npp:MenuCommand(IDM_FILE_EXIT)
   end
end

local function nextFile()
   local fileAvail = false
   if outFile ~= nil then
     io.close(outFile)
   end
   while (not fileAvail) and (testFiles[nFile] ~= nil) do
      local fileName = npp:GetNppDirectory() .. "\\..\\Test\\UrlDetection\\" .. testFiles[nFile]
      fileAvail = npp:SwitchToFile(fileName)
      if not fileAvail then
         local f = io.open(fileName,"r")
         if f~=nil then
            io.close(f)
            fileAvail = npp:DoOpen(fileName)
         end
      end
      -- print("Verifying " .. testFiles[nFile] .. " ...")
      print("Verifying " .. npp:GetFileName() .. " ...")
      if fileAvail then
         local outFileName = fileName .. ".result"
         outFile = io.open(outFileName,"w")
         if outFile == nil then
            testResults[nFile] = "KO"
            print("KO", "Cannot open output file \""..fileName.."\"")
            print()
            nFile = nFile + 1;
         end
      else
         testResults[nFile] = "KO"
         print("KO", "Cannot open file \""..fileName.."\"")
         print()
         nFile = nFile + 1;
      end
   end
   return fileAvail
end

local function scrollToNextURL()
   editor.TargetStart = curPos
   editor.TargetEnd = editor.Length
   editor.SearchFlags = SCFIND_REGEXP
   local iRes = editor:SearchInTarget("^u .+ u$")
   if iRes >= 0 then
      uFrom = editor.TargetStart
      uTo = editor.TargetEnd
      editor.TargetStart = uFrom
      editor.TargetEnd = editor.Length
      iRes = editor:SearchInTarget("^m .+ m$")
      if iRes >= 0 then
         mFrom = editor.TargetStart
         mTo = editor.TargetEnd
         local ln1 = editor:LineFromPosition(uFrom)
         local ln2 = editor:LineFromPosition(mFrom)
         if (ln1+1) == ln2 then
            editor:ScrollRange(mTo, uFrom)
            return 1
         else
            editor:GotoPos(mFrom)
            OKorKO = "KO"
            print("KO", "Mask line not following immediately after URL line")
            return -1
         end
      else
         OKorKO = "KO"
         print ("KO", "Mask line not found")
         return -1
      end
   else
      return 0
   end
end

local function verifyURL()
   local mMsk = editor:textrange(mFrom, mTo)
   editor:GotoPos(uFrom + 2)
   local uMsk = "m "
   local limit = mTo - mFrom -- if something goes wrong, edit.CurrentPos may never reach (uTo - 2).
   while (editor.CurrentPos < uTo - 2) and (limit >= 0) do
      if editor:IndicatorValueAt(URL_INDIC, editor.CurrentPos) == 0 then
         uMsk = uMsk .. "0"
      else
         uMsk = uMsk .. "1"
      end
      editor:CharRight()
      limit = limit - 1
   end
   local Res = 0
   if limit >= 0 then
      if editor:textrange(editor.CurrentPos, editor.CurrentPos + 2) == " u" then
         uMsk = uMsk .. " m"
         if uMsk == mMsk then
            outFile:write("OK", "\t", editor:textrange(uFrom, uTo), "\n")
            Res = 1
         else
            outFile:write("KO", "\t", editor:textrange(uFrom, uTo), "\n")
            outFile:write("ok", "\t", mMsk, "\n")
            outFile:write("ko", "\t", uMsk, "\n")
            print("KO", "\t", editor:textrange(uFrom, uTo))
            print("ok", "\t", mMsk)
            print("ko", "\t", uMsk)
            OKorKO = "KO"
            Res = 1
         end
      end
   else
      outFile:write("KO", "\t", "internal error", "\n")
      OKorKO = "KO"
   end
   return Res
end

local function goForward(timer)
   if task < 0 then
      task = task + 1
      if task == 0 then
         if not nextFile() then
            npp.StopTimer(timer)
            Summary()
         end
      end
   elseif task == 0 then
      local urlAvail = scrollToNextURL()
      if urlAvail == 1 then
         task = 1
      else
         npp.StopTimer(timer)
         print(OKorKO)
         print()
         testResults[nFile] = OKorKO
         if urlAvail == 0 then
            nFile = nFile + 1
            if nextFile() then
               task = 0
               curPos = 0
               OKorKO = "OK"
               npp.StartTimer(timerInterval, goForward)
            else
               Summary()
            end
         else
            Summary()
         end
      end
   elseif task == 1 then
      if verifyURL() == 0 then
         npp.StopTimer(timer)
         print()
         Summary()
      else
         curPos = mTo
         task = 0
      end
   else
      npp.stopTimer(timer)
      print("KO", "Internal impossibility")
      print()
      Summary()
   end
end

npp.ClearConsole()
npp.StartTimer(timerInterval, goForward)

