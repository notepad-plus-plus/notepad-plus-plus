$testRoot = ".\"

Get-ChildItem -Path $testRoot -Attribute Directory -Name |
Foreach-Object {

		$dirName = (Get-Item $testRoot$_).Name
		$langName = $dirName
		$sw = [Diagnostics.Stopwatch]::StartNew()
		$result = & ".\unitTest.ps1" $dirName $langName
		$sw.Stop()
		"Test: " + $sw.Elapsed.TotalMilliseconds + " ms"


		if ($result -eq 0)
		{
			"$dirName ... OK"
		}
		elseif ($result -eq 1)
		{
			"$dirName ... unitTest file not found. Test skipped."
		}
		elseif ($result -eq -1)
		{
			"$dirName ... KO"
			"result = $result"
			"There are some problems in your $dirName.xml"
			exit -1
		}
		elseif ($result -eq -2)
		{
			"Exception!"
			exit -1
		}
		else
		{
			"It should not happen - check your script."
			exit -1
		}

		# Check all Sub-directories for other unit-tests
		Get-ChildItem -Path $testRoot\$dirName -Attribute Directory -Name |
		Foreach-Object {

				$subDirName = (Get-Item $testRoot$dirName\$_).Name
				$sw = [Diagnostics.Stopwatch]::StartNew()
				$subResult = &.\unitTest.ps1 $langName\$subDirName $langName
				$sw.Stop()
				"Test:" + $sw.Elapsed.TotalMilliseconds + " ms"
				if ($subResult -eq 0)
				{
					"$dirName-$subDirName ... OK"
				}
				elseif ($subResult -eq 1)
				{
					"$dirName-$subDirName ... unitTest file not found. Test skipped."
				}
				else
				{
					"$dirName-$subDirName ... KO"
					""
					"There are some problems in your $dirName.xml"
					exit -1
				}
		}
}
""
"All tests are passed."
exit 0