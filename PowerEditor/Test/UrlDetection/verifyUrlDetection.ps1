try {
	if (Test-Path -Path '..\..\Bin\plugins' -PathType Container)
	{
		if (Test-Path -Path '..\..\Bin\plugins_save' -PathType Container)
		{
			"Backup for plugins directory already exists"
			exit -1
		}
		"Backing up plugin directory ..."
		Move-Item ..\..\Bin\plugins ..\..\bin\plugins_save
	}
	"Installing Lua plugin for testing ..."
	Copy-Item -Path .\plugins -Destination ..\..\bin -Recurse

	"Testing ..."
	..\..\bin\notepad++.exe | Out-Null

	if (Test-Path -Path '..\..\Bin\plugins_save' -PathType Container)
	{
		"Removing Lua plugin ..."
		Remove-Item -Path ..\..\Bin\plugins -Recurse -Force
		"Restoring plugin directory ..."
		Move-Item ..\..\Bin\plugins_save ..\..\bin\plugins
	}

	$expectedRes = Get-Content .\verifyUrlDetection_1a.expected.result
	$generatedRes = Get-Content .\verifyUrlDetection_1a.result

	if (Compare-Object -ReferenceObject $expectedRes -DifferenceObject $generatedRes)
	{
		"Unexpected test results for verifyUrlDetection_1a"
		exit -1
	}
	else
	{
		Remove-Item .\verifyUrlDetection_1a.result
		$expectedRes = Get-Content .\verifyUrlDetection_1b.expected.result
		$generatedRes = Get-Content .\verifyUrlDetection_1b.result
		if (Compare-Object -ReferenceObject $expectedRes -DifferenceObject $generatedRes)
		{
			"Unexpected test results for verifyUrlDetection_1b"
			exit -1
		}
		else
		{
			Remove-Item .\verifyUrlDetection_1b.result
			"URL detection test OK"
			exit 0
		}
	}
}
catch
{
	"Unexpected behavior while URL detection test"
	exit -1
}

