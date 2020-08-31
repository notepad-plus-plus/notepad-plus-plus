$testRoot = ".\"

Get-ChildItem $testRoot -Filter *.* | 
Foreach-Object {
    if ((Get-Item $testRoot$_) -is [System.IO.DirectoryInfo])
    {
        $dirName = (Get-Item $testRoot$_).Name
		$langName = $dirName
        $result = &.\unitTest.ps1 $dirName $langName
		
        if ($result -eq 0)
		{
			"$dirName ... OK"
		}
        elseif ($result -eq 1)
		{		
			"$dirName ... unitTest file not found. Test skipt."
		}
        else
        {
			"$dirName ... KO"
			""
			"There are some problems in your functionList.xml"
            exit -1
        }
		
		# Check all Sub-directories for other unit-tests
		Get-ChildItem $testRoot$dirName -Filter *.* | 
		Foreach-Object {
		    if ((Get-Item $testRoot$dirName\$_) -is [System.IO.DirectoryInfo])
			{
				$subDirName = (Get-Item $testRoot$dirName\$_).Name
				$subResult = &.\unitTest.ps1 $langName\$subDirName $langName
				
				if ($subResult -eq 0)
				{
					"$dirName-$subDirName ... OK"
				}
				elseif ($subResult -eq 1)
				{		
					"$dirName-$subDirName ... unitTest file not found. Test skipt."
				}
				else
				{
					"$dirName-$subDirName ... KO"
					""
					"There are some problems in your functionList.xml"
					exit -1
				}
			}
		}
    }
}
""
"All tests are passed."
exit 0