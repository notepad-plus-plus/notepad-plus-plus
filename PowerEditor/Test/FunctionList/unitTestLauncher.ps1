..\..\bin\notepad++.exe -export=functionList -lasm .\asm\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lautoit .\autoit\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lbash .\bash\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lbatch .\batch\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lc .\c\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lcpp .\cpp\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lcs .\cs\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lini .\ini\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -linno .\inno\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -ljava .\java\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -ljavascript .\javascript\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lnsis .\nsis\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lperl .\perl\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lphp .\php\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lpowershell .\powershell\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lpython .\python\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lruby .\ruby\unitTest | Out-Null
..\..\bin\notepad++.exe -export=functionList -lxml .\xml\unitTest | Out-Null


$testRoot = ".\"

Get-ChildItem $testRoot -Filter *.* | 
Foreach-Object {
    if ((Get-Item $testRoot$_) -is [System.IO.DirectoryInfo])
    {        
        $expectedRes = Get-Content $testRoot$_\unitTest.expected.result
        $generatedRes = Get-Content $testRoot$_\unitTest.result.json
        
        if ($generatedRes -eq $expectedRes)
        {
           Remove-Item $testRoot$_\unitTest.result.json
           ""
           "OK"
        }
        else
        {
            "$generatedRes"
            exit -1
        }
        
    }
}

exit 0

