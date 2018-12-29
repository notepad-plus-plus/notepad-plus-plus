$testRoot = ".\"

$dirName=$args[0]
$langName=$args[1]


if ((Get-Item $testRoot$dirName) -is [System.IO.DirectoryInfo])
{
	..\..\bin\notepad++.exe -export=functionList -l"$langName" $testRoot$dirName\unitTest | Out-Null

	$expectedRes = Get-Content $testRoot$dirName\unitTest.expected.result
	$generatedRes = Get-Content $testRoot$dirName\unitTest.result.json
	
	if ($generatedRes -eq $expectedRes)
	{
	   Remove-Item $testRoot$dirName\unitTest.result.json
	   "$dirName ... OK"
	}
	else
	{
		"$dirName ... KO"
		""
		"There's a (some) problem(s) in your functionList.xml"
		exit -1
	}	
}
