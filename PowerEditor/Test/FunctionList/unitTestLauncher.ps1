$testRoot = ".\"

Get-ChildItem $testRoot -Filter *.* | 
Foreach-Object {
    if ((Get-Item $testRoot$_) -is [System.IO.DirectoryInfo])
    {
        $dirName = (Get-Item $testRoot$_).Name
        .\unitTest.ps1 $dirName $dirName
        
    }
}
""
"All tests are passed."
exit 0