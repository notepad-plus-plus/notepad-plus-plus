
$testRoot = ".\"

Get-ChildItem $testRoot -Filter *.* | 
Foreach-Object {
    if ((Get-Item $testRoot$_) -is [System.IO.DirectoryInfo])
    {
        $dirName = (Get-Item $testRoot$_).Name
        ..\..\bin\notepad++.exe -export=functionList -l"$dirName" $testRoot$_\unitTest | Out-Null

        $expectedRes = Get-Content $testRoot$_\unitTest.expected.result
        $generatedRes = Get-Content $testRoot$_\unitTest.result.json
        
        if ($generatedRes -eq $expectedRes)
        {
           Remove-Item $testRoot$_\unitTest.result.json
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
}
""
"All tests are passed."
exit 0
