# This script does 1 unit-test on given relative dir path and on given language.
# Here's its syntax:
# .\unit-test.ps1 RELATIVE_PATH LANG
# It return 0 if result is OK
#          -1 if result is KO
#          -2 if exception
#           1 if unitTest file not found


$testRoot = ".\"

$dirName=$args[0]
$langName=$args[1]

Try {
	if ((Get-Item $testRoot$dirName) -is [System.IO.DirectoryInfo])
	{
		if (-Not (Test-Path $testRoot$dirName\unitTest))
		{
			return 1
		}
		if ($langName.StartsWith("udl-"))
		{
			$langName = $langName.Replace("udl-", "")
			..\..\bin\notepad++.exe -export=functionList -udl="`"$langName"`" $testRoot$dirName\unitTest | Out-Null
		}
		else
		{
			..\..\bin\notepad++.exe -export=functionList -l"$langName" $testRoot$dirName\unitTest | Out-Null
		}

		$expectedRes = Get-Content $testRoot$dirName\unitTest.expected.result
		$generatedRes = Get-Content $testRoot$dirName\unitTest.result.json
		
		if ($generatedRes -eq $expectedRes)
		{
		   Remove-Item $testRoot$dirName\unitTest.result.json
		   return 0
		}
		else
		{
			return -1
		}	
	}
}
Catch
{
	return -2
}
